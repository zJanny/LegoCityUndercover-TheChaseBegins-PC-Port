// Cast resolution of the cutscene object builder 0x2d1ddc (docs/re/cutscene-actors.md). The folder and model names are those of the
// shipped cutscene 1_1_a; the level characters' CSCharacter values are read from lvl_01 by the real-data test below.
#include "assets/assets.hpp"
#include "runtime/cutscene_cast.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::runtime::CastTargetKind;
using chase::runtime::resolveCastTarget;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void synthetic() {
    const std::vector<std::string> names = {"trolley_guy1", "DonutMan", "", "DonutEater"};
    auto target = resolveCastTarget("donutman", "donutman_minifig.bfnmdl", true, names);
    require(target.kind == CastTargetKind::LevelCharacter && target.characterIndex == 1, "folder compares case-insensitively with CSCharacter");
    target = resolveCastTarget("DonutEater", "donuteater_minifig.bfnmdl", true, names);
    require(target.kind == CastTargetKind::LevelCharacter && target.characterIndex == 3, "second character");
    target = resolveCastTarget("player1", "player1_minifig.bfnmdl", true, names);
    require(target.kind == CastTargetKind::Player, "player1 minifig is the player");
    target = resolveCastTarget("player1", "player1_minifig.bfnmdl", false, names);
    require(target.kind == CastTargetKind::Standalone, "no player, own model");
    target = resolveCastTarget("player1", "player1_cape.bfnmdl", true, names);
    require(target.kind == CastTargetKind::Standalone, "capes are own models");
    target = resolveCastTarget("player2", "player2_minifig.bfnmdl", true, names);
    require(target.kind == CastTargetKind::Standalone, "only Player1 exists");
    target = resolveCastTarget("player1", "player1_prop.bfnmdl", true, names);
    require(target.kind == CastTargetKind::Standalone, "player folders need a minifig model");
    target = resolveCastTarget("Donut1", "donut1.bfnmdl", true, names);
    require(target.kind == CastTargetKind::Standalone, "no CSCharacter match, own model");
    target = resolveCastTarget("", "x_minifig.bfnmdl", true, names);
    require(target.kind == CastTargetKind::Standalone, "empty folder never matches an unset CSCharacter");
}

void realData(const char* fibPath, const char* namesPath) {
    using namespace chase::assets;
    FuseArchive archive(fibPath);
    archive.loadNames(namesPath);
    const FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(entry != nullptr, "level in archive");
    const Level level = parseLevel(archive.read(*entry));
    const DrawableObjects drawable = drawableObjects(archive, level);
    std::vector<std::string> names;
    for (const CharacterObject& character : drawable.characters) names.push_back(character.cutsceneName);
    for (const char* folder : {"DonutMan", "DonutEater"}) {
        const auto target = resolveCastTarget(folder, std::string(folder) + "_minifig.bfnmdl", true, names);
        require(target.kind == CastTargetKind::LevelCharacter, std::string(folder) + " has a level character");
        const std::string instance = drawable.characters[target.characterIndex].placement.instance;
        require(instance == (std::string(folder) == "DonutMan" ? "1-1_AI_DonutShopMan" : "1-1_AI_DonutShopMan01"), "instance of " + std::string(folder));
    }
    const CutsceneContent content = loadCutscene(archive, "1_1_a");
    const Skeleton shared = parseSkeleton(archive.read(*archive.find("models/char_minifig/char_minifig.bfnskl")));
    for (const CutsceneCast& cast : content.actors) {
        if (cast.skeleton.joints.empty()) continue;
        require(cast.skeleton.joints.size() == shared.joints.size(), "cast skeleton has the joints of char_minifig: " + cast.model);
        for (std::size_t i = 0; i < shared.joints.size(); ++i)
            require(cast.skeleton.joints[i].name == shared.joints[i].name, "joint " + shared.joints[i].name + " of " + cast.model);
    }
}
}

int main(int argc, char** argv) {
    try {
        synthetic();
        if (argc == 3 && std::filesystem::exists(argv[1]) && std::filesystem::exists(argv[2])) realData(argv[1], argv[2]);
        else std::cout << "cutscene-cast-test: real data not available, synthetic part only\n";
    } catch (const std::exception& error) {
        std::cerr << "cutscene-cast-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "cutscene-cast-test ok\n";
    return 0;
}
