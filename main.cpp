// SPDX-License-Identifier: MIT
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include <assimp/cimport.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <boost/program_options.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4

namespace po = boost::program_options;

struct Mesh
{
  glm::vec3 color;

  uint32_t vertex_base;
  uint32_t vertex_count;
  uint32_t index_base;
  uint32_t index_count;
};

struct Vertex
{
  glm::vec3 position;
  glm::vec3 normal;
};

void write_to_file(std::ofstream &output_file, const uint32_t &ui)
{
  output_file.write((char*)&ui, sizeof(uint32_t));
}

void write_to_file(std::ofstream &output_file, const glm::vec3 &vector)
{
  output_file.write((char*)&vector.x, sizeof(glm::vec3::value_type));
  output_file.write((char*)&vector.y, sizeof(glm::vec3::value_type));
  output_file.write((char*)&vector.z, sizeof(glm::vec3::value_type));
}

int main(int argc, char *argv[])
{
  std::string file_path;
  std::ofstream output_file;

  // Get arguments.
  {
    bool incomplete_arguments{false};

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "show this help message")
        ("source", po::value<std::string>(), "file to be imported")
        ("out", po::value<std::string>(), "exported file name")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
      std::cout << desc << "\n";
      return 0;
    }

    if (vm.count("source"))
    {
      file_path = vm["source"].as<std::string>();
    }
    else
    {
      incomplete_arguments = true;
      std::cout << "Source was not set.\n";
    }

    if (vm.count("out"))
    {
      output_file.open(vm["out"].as<std::string>());
    }
    else
    {
      incomplete_arguments = true;
      std::cout << "Output was not set.\n";
    }

    if(incomplete_arguments) return 0;
  }

  // Export file.
  {
    Assimp::Importer importer{};
    const aiScene* p_scene{nullptr};

    std::vector<Mesh> meshes{};
    std::vector<Vertex> vertexes{};
    std::vector<uint32_t> indexes{};

    p_scene = importer.ReadFile(file_path.c_str(),
                                aiProcess_Triangulate |
                                aiProcess_JoinIdenticalVertices |
                                aiProcess_SortByPType);

    if(p_scene == nullptr)
    {
      std::cout << "Failed to load model. Assimp error: " << aiGetErrorString()
                << std::endl;

      return 0;
    }

    meshes.resize(p_scene->mNumMeshes);

    // Iterate over meshes.
    for(uint32_t mesh_num{0}; mesh_num < p_scene->mNumMeshes; mesh_num++)
    {
      const aiMesh* p_ai_mesh{p_scene->mMeshes[mesh_num]};

      meshes[mesh_num].vertex_base = vertexes.size();
      meshes[mesh_num].index_base = indexes.size();

      meshes[mesh_num].vertex_count = p_ai_mesh->mNumVertices;

      aiColor3D ai_color{0.0f, 0.0f, 0.0f};
      p_scene->mMaterials[p_ai_mesh->mMaterialIndex]->Get(
          AI_MATKEY_COLOR_DIFFUSE, ai_color);

      meshes[mesh_num].color.r = ai_color.r;
      meshes[mesh_num].color.g = ai_color.g;
      meshes[mesh_num].color.b = ai_color.b;

      const aiVector3D zero3d{0.0f, 0.0f, 0.0f};

      // FIXME: assimp is multiplicating vertexes.
      // Load vertexes.
      for(uint32_t vertex_num{0}; vertex_num < p_ai_mesh->mNumVertices;
          vertex_num++)
      {
        /*
          Get data from model.
        */

        const aiVector3D* p_position;
        const aiVector3D* p_normal;
        const aiVector3D* p_texture_coord;

        Vertex vert;

        p_position = &(p_ai_mesh->mVertices[vertex_num]);
        p_normal = &(p_ai_mesh->mNormals[vertex_num]);
        p_texture_coord =
            (p_ai_mesh->HasTextureCoords(0)) ?
            &(p_ai_mesh->mTextureCoords[0][vertex_num]) : &zero3d;

        /*
          Save data.
        */

        vert.position.x = p_position->x;
        vert.position.y = p_position->y;
        vert.position.z = p_position->z;

        vertexes.push_back(vert);
      }

      // Get number of vertices from previous meshes.
      uint32_t index_base{static_cast<uint32_t>(indexes.size())};

      // Load faces.
      for(uint32_t face_num{0}; face_num < p_ai_mesh->mNumFaces; face_num++)
      {
        const aiFace& ai_face{p_ai_mesh->mFaces[face_num]};

        // Ignore any polygon that is not a triangle.
        if (ai_face.mNumIndices != 3) continue;

        indexes.push_back(index_base + ai_face.mIndices[0]);
        indexes.push_back(index_base + ai_face.mIndices[1]);
        indexes.push_back(index_base + ai_face.mIndices[2]);

        meshes[mesh_num].index_count += 3;
      }
    }

    /*
      Print information about model.
    */

    std::cout << "Vertex count: " << vertexes.size() << std::endl;
    std::cout << "Index count: " << indexes.size() << std::endl;

    std::cout << "Meshes: " << meshes.size() << std::endl;
    for(auto mesh: meshes)
    {
      std::cout << "Color: r: " << mesh.color.r << ", g: " << mesh.color.g <<
          ", b:" << mesh.color.b << std::endl;

      std::cout << "Vertex base: " << mesh.vertex_base << std::endl;
      std::cout << "Vertex count: " << mesh.vertex_count << std::endl;

      std::cout << "Index base: " << mesh.index_base << std::endl;
      std::cout << "Index count: " << mesh.index_count << std::endl;

      std::cout << std::endl;
    }

    // Save meshes.
    {
      uint32_t meshes_count{meshes.size()};
      write_to_file(output_file, meshes_count);

      for(auto mesh: meshes)
      {
        write_to_file(output_file, mesh.color);

        write_to_file(output_file, mesh.vertex_base);
        write_to_file(output_file, mesh.vertex_count);
        write_to_file(output_file, mesh.index_base);
        write_to_file(output_file, mesh.index_count);
      }
    }

    // Save vertexes on file.
    {
      uint32_t vertex_count{vertexes.size()};
      write_to_file(output_file, vertex_count);

      for(auto vertex: vertexes)
      {
        write_to_file(output_file, vertex.position);
        write_to_file(output_file, vertex.normal);
      }
    }

    // Save indexes on file.
    {
      uint32_t index_count{indexes.size()};
      write_to_file(output_file, index_count);

      for(auto index: indexes) write_to_file(output_file, index);
    }
  }

  return 0;
}
