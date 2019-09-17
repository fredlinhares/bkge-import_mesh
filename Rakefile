# BlueKitty game engine mesh importer
name = "bkgeim"

sources = "main.cpp"

links = %{-lassimp -lboost_program_options}

task default: %w[build]

task :build do
  `g++ #{sources} -w #{links} -o #{name}`
end
