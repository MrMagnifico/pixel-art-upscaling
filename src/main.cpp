#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
DISABLE_WARNINGS_POP()
#include <framework/image.h>

#include "common.hpp"
#include "2xsai.hpp"
#include "eagle.hpp"
#include "epx.hpp"
#include "hq2x.hpp"
#include "nedi.hpp"
#include "xbr.hpp"

static const std::filesystem::path data_dir_path { DATA_DIR };
static const std::filesystem::path out_dir_path { OUTPUT_DIR };

// Is there a better way to do this? Yes.
// Do I have the time? Questionable.
static const std::vector<std::string> TEST_FILES = {
    "SonictheHedgehog_SonicSprite",
    "Z-Saber_Zero_MX3",
    "gaxe_skeleton_input",
    "sbm1_02_input",
    "sma_chest_input",
    "sma_peach_01_input",
    "smw2_yoshi_01_input",
    "smw2_yoshi_02_input",
    "smw_boo_input",
    "smw_bowser_input",
    "smw_dolphin_input",
    "smw_help_input",
    "smw_mario_input",
    "smw_mushroom_input"};

int main(int argc, char** argv) {
    #pragma omp parallel for
    for (const std::string& filename : TEST_FILES) {
        Image<glm::uvec3> input     = Image<glm::uvec3>(data_dir_path / (filename + ".png"));
        Image<glm::vec3> input_flt  = Image<glm::vec3>(data_dir_path / (filename + ".png"));
        input.writeToFile(out_dir_path / (filename + "-initial_image.png"));

        Image<glm::uvec3> scale_epx         = input;
        Image<glm::uvec3> scale_adv_mame    = input;
        Image<glm::uvec3> scale_eagle       = input;
        Image<glm::uvec3> scale_2xsai       = input;
        Image<glm::uvec3> scale_hq2x        = input;
        Image<glm::uvec3> scale_xbr         = input;
        Image<glm::vec3> scale_nedi         = input_flt;

        for (uint32_t scale_factor = 2U; scale_factor <= 16U; scale_factor *= 2) {
            std::cout << "Scaling "<< filename << " by " << scale_factor << "x..." << std::endl;

            scale_epx       = scaleEpx(scale_epx);
            scale_adv_mame  = scaleAdvMame(scale_adv_mame);
            scale_eagle     = scaleEagle(scale_eagle);
            scale_2xsai     = scale2xSaI(scale_2xsai);
            scale_hq2x      = scaleHq2x(scale_hq2x);
            scale_xbr       = scaleXbr(scale_xbr);
            scale_nedi      = scaleNedi(scale_nedi);
            
            scale_epx.writeToFile(out_dir_path / (filename + "-scale_epx-" + std::to_string(scale_factor) + "X.png"));
            scale_adv_mame.writeToFile(out_dir_path / (filename + "-scale_adv_mame-" + std::to_string(scale_factor) + "X.png"));
            scale_eagle.writeToFile(out_dir_path / (filename + "-scale_eagle-" + std::to_string(scale_factor) + "X.png"));
            scale_2xsai.writeToFile(out_dir_path / (filename + "-scale_2xSaI-" + std::to_string(scale_factor) + "X.png"));
            scale_hq2x.writeToFile(out_dir_path / (filename + "-scale_hq2x-" + std::to_string(scale_factor) + "X.png"));
            scale_xbr.writeToFile(out_dir_path / (filename + "-scale_xbr-" + std::to_string(scale_factor) + "X.png"));
            scale_nedi.writeToFile(out_dir_path /(filename + "-scale_nedi-" + std::to_string(scale_factor) + "X.png"));
        }
    }
    
    return EXIT_SUCCESS;
}
