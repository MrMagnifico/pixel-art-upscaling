#include <filesystem>
#include <fstream>
#include <iostream>

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

int main(int argc, char** argv) {
    Image<glm::uvec3> input = Image<glm::uvec3>(data_dir_path / "Z-Saber_Zero_MX3.png");
    Image<glm::vec3> input_flt = Image<glm::vec3>(data_dir_path / "Z-Saber_Zero_MX3.png");
    input.writeToFile(out_dir_path / "initial_image.png");

    Image<glm::uvec3> scale_epx = scaleEpx(input);
    Image<glm::uvec3> scale_adv_mame = scaleAdvMame(input);
    Image<glm::uvec3> scale_eagle = scaleEagle(input);
    Image<glm::uvec3> scale_2xsai = scale2xSaI(input);
    Image<glm::uvec3> scale_hq2x = scaleHq2x(input);
    Image<glm::uvec3> scale_xbr = scaleXbr(input);
    Image<glm::vec3> scale_nedi = scaleNedi(input_flt);

    scale_epx.writeToFile(out_dir_path / "scale_epx.png");
    scale_adv_mame.writeToFile(out_dir_path / "scale_adv_mame.png");
    scale_eagle.writeToFile(out_dir_path / "scale_eagle.png");
    scale_2xsai.writeToFile(out_dir_path / "scale_2xSaI.png");
    scale_hq2x.writeToFile(out_dir_path / "scale_hq2x.png");
    scale_xbr.writeToFile(out_dir_path / "scale_xbr.png");
    scale_nedi.writeToFile(out_dir_path / "scale_nedi.png");

    return EXIT_SUCCESS;
}
