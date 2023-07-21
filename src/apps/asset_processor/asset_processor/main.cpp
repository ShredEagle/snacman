#include "Processor.h"
#include "Logging.h"

#include <filesystem>
#include <iostream>

#include <cstdlib>


int main(int argc, char * argv[])
{
    if(argc != 2)
    {
        std::cerr << "Usage: " << std::filesystem::path{argv[0]}.filename().string() << " input_model_file\n";
        return EXIT_FAILURE;
    }

    std::filesystem::path inputPath{argv[1]};

    if(!is_regular_file(inputPath))
    {
        std::cerr << "Provided argument should be a file.\n";
        return EXIT_FAILURE;
    }

    ad::initializeLogging();
    ad::renderer::processModel(inputPath);

    return EXIT_SUCCESS;
}