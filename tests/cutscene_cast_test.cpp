// Cast resolution of the cutscene object builder 0x2d1ddc (docs/re/cutscene-actors.md). The folder and model names are those of the
// shipped cutscene 1_1_a; the level characters' CSCharacter values are read from lvl_01 by the real-data test below.
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
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
    require(target.kind == CastTargetKind::LevelCharacter && target.index == 1, "folder compares case-insensitively with CSCharacter");
    target = resolveCastTarget("DonutEater", "donuteater_minifig.bfnmdl", true, names);
    require(target.kind == CastTargetKind::LevelCharacter && target.index == 3, "second character");
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

void props() {
    const std::vector<chase::runtime::CastEntity> entities = {{"hoop", true, 0}, {"Dog", false, 3}, {"dog", true, 1}, {"key", true, 2}};
    auto target = resolveCastTarget("dog", "dog.bfnmdl", true, entities);
    require(target.kind == CastTargetKind::LevelCharacter && target.index == 3, "the first entity in entity order wins whatever its kind");
    target = resolveCastTarget("KEY", "key.bfnmdl", true, entities);
    require(target.kind == CastTargetKind::LevelProp && target.index == 2, "a prop is matched case-insensitively");
    target = resolveCastTarget("hoop", "hoop_cape.bfnmdl", true, entities);
    require(target.kind == CastTargetKind::Standalone, "capes never match, props included");
    target = resolveCastTarget("player1", "player1_minifig.bfnmdl", true, entities);
    require(target.kind == CastTargetKind::Player, "player folders never look at props");
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
        const std::string instance = drawable.characters[target.index].placement.instance;
        require(instance == (std::string(folder) == "DonutMan" ? "1-1_AI_DonutShopMan" : "1-1_AI_DonutShopMan01"), "instance of " + std::string(folder));
    }
    {
        const ClassLookup classOf = archiveClassLookup(archive);
        std::vector<chase::runtime::CastEntity> entities;
        std::vector<std::string> instances;
        for (std::size_t i = 0; i < level.objects.size(); ++i) {
            const std::string name = objectCutsceneCharacter(level.objects[i], *classOf(level.objects[i].className));
            if (name.empty() || name == "player1") continue;
            entities.push_back({name, true, instances.size()});
            instances.push_back(level.objects[i].instance);
        }
        // Recorded: scan of lvl_01 for CSCharacter (docs/re/cutscene-actors.md); 1_2_c lists the folders dog, dog_owner, policebarker, prop_heart.
        const auto dog = resolveCastTarget("dog", "dog.bfnmdl", true, entities);
        require(dog.kind == CastTargetKind::LevelProp && instances[dog.index] == "1-2_Dog", "folder dog is the entity 1-2_Dog");
        const CutsceneContent dogCutscene = loadCutscene(archive, "1_2_c");
        const Skeleton dogSkeleton = parseSkeleton(archive.read(*archive.find("models/char_dog/char_dog.bfnskl")));
        bool found = false;
        for (const CutsceneCast& cast : dogCutscene.actors) {
            if (cast.model != "csdata/models/1_2_c/dog/dog.bfnmdl") continue;
            found = true;
            require(cast.skeleton.joints.size() == dogSkeleton.joints.size() && dogSkeleton.joints.size() == 19, "the cutscene dog has the 19 joints of char_dog");
            for (std::size_t i = 0; i < dogSkeleton.joints.size(); ++i) require(cast.skeleton.joints[i].name == dogSkeleton.joints[i].name, "dog joint " + dogSkeleton.joints[i].name);
            require(cast.clips.size() == 1 && cast.clips[0].startFrame == 180, "the dog clip of 1_2_c starts at camera frame 180");
        }
        require(found, "1_2_c has the dog cast");
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
        props();
        if (argc == 3 && std::filesystem::exists(argv[1]) && std::filesystem::exists(argv[2])) realData(argv[1], argv[2]);
        else std::cout << "cutscene-cast-test: real data not available, synthetic part only\n";
    } catch (const std::exception& error) {
        std::cerr << "cutscene-cast-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "cutscene-cast-test ok\n";
    return 0;
}
