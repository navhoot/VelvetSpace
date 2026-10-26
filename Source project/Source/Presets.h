#pragma once
#include <vector>
#include <utility>

namespace velvet {
// A factory modulation lane: source index (see laneSources), destination parameter id, direction, depth %.
struct LaneDef { int source; const char* dest; int direction; float depth; };
struct FactoryPreset {
    const char* name; const char* category;
    std::vector<std::pair<const char*, float>> values; // parameter id -> plain value (everything else = default)
    std::vector<LaneDef> lanes;                          // fills lanes 1..n
};

// Reverb types: 0 Room 1 Chamber 2 Plate 3 Hall 4 Vintage 80s 5 Bloom 6 Cathedral 7 Spring 8 Dark Space 9 Nebula
// Select mode: 0 Snap 1 Blend 2 Cycle.  Saturation: 0 Clean 1 Tape 2 Tube 3 Soft 4 Hard 5 Fold 6 Rectify 7 Asym 8 Bitcrush 9 Downsample
// Sync: 0 Free 3 1/16 4 1/8 5 1/4 6 1/2 7 1 bar 8 2 bars 9 4 bars 10 8 bars.  Lane sources: 1 Audio Env 2 MIDI Env 3-6 LFO 1-4 7-10 Macro 1-4
inline const std::vector<FactoryPreset>& factoryPresets() {
    static const std::vector<FactoryPreset> p = {
    { "Clean Canvas", "Utility", {}, {} },
    { "Velvet Hall", "Halls",
      { {"rtype",3},{"selmode",0},{"mix",30},{"size",70},{"decay",3.9f},{"predelay",28},{"damping",7200},{"diffusion",84},{"lowcut",140} },
      { {3,"damping",0,8} } },
    { "1984 Concert Hall", "Halls",
      { {"rtype",4},{"selmode",0},{"mix",33},{"size",86},{"decay",5.6f},{"predelay",34},{"damping",5200},{"highcut",9800},{"moddepth",1.3f},{"modrate",.6f},{"width",125},
        {"lowtype",1},{"midtype",1},{"lowdrive",6},{"middrive",8},{"satmix",60} },
      { {4,"size",0,6} } },
    { "Bloom Garden", "Ambient",
      { {"rtype",5},{"selmode",1},{"mix",42},{"size",78},{"decay",7.5f},{"predelay",40},{"damping",9000},{"diffusion",90},{"width",140},{"eqon",1},{"eqhigh",-3},{"eqlow",-2} },
      { {3,"rtype",0,4},{1,"mix",2,18} } },
    { "Bright Plate", "Plates",
      { {"rtype",2},{"selmode",0},{"mix",25},{"size",42},{"decay",2.2f},{"predelay",8},{"damping",16000},{"diffusion",96},{"lowcut",220},{"treblemult",.85f},{"bassmult",.8f} }, {} },
    { "Vocal Plate Ducked", "Plates",
      { {"rtype",2},{"selmode",0},{"mix",34},{"size",50},{"decay",2.6f},{"predelay",42},{"damping",11000},{"lowcut",260},
        {"duckdepth",14},{"duckthresh",-30},{"duckattack",6},{"duckrelease",420} }, {} },
    { "Tight Drum Room", "Rooms",
      { {"rtype",0},{"selmode",0},{"mix",22},{"size",28},{"decay",.8f},{"predelay",2},{"damping",8500},{"early",-2},
        {"midtype",2},{"hightype",3},{"middrive",12},{"highdrive",7},{"satmix",55} }, {} },
    { "Wood Chamber", "Chambers",
      { {"rtype",1},{"selmode",0},{"mix",27},{"size",52},{"decay",1.7f},{"predelay",14},{"damping",6000},{"bassmult",1.25f},{"treblemult",.55f} }, {} },
    { "Cathedral Wash", "Huge",
      { {"rtype",6},{"selmode",0},{"mix",45},{"size",100},{"decay",14},{"predelay",60},{"damping",4800},{"moddepth",1.8f},{"modrate",.18f},{"width",140},{"lowcut",160} },
      { {3,"moddepth",0,20} } },
    { "Surf Spring", "Vintage",
      { {"rtype",7},{"selmode",0},{"mix",30},{"size",40},{"decay",2.4f},{"predelay",4},{"damping",6500},{"lowcut",300},{"highcut",7000},
        {"midtype",1},{"middrive",9},{"satmix",70} }, {} },
    { "Dark Space Drone", "Ambient",
      { {"rtype",8},{"selmode",0},{"mix",55},{"size",95},{"decay",18},{"predelay",80},{"damping",2600},{"width",160},{"eqon",1},{"eqlow",3},{"eqhigh",-6} },
      { {3,"around",0,0} } },
    { "Nebula Cycle", "Morphing",
      { {"rtype",5},{"selmode",2},{"cyclerate",.05f},{"cyclespan",5},{"cyclestyle",1},{"mix",45},{"size",80},{"decay",9},{"predelay",30},{"width",150},
        {"orbiton",1},{"orbitmode",0},{"orbitrate",.08f},{"around",70},{"updown",35},{"nearfar",40} }, {} },
    { "Endless Morph", "Morphing",
      { {"rtype",0},{"selmode",2},{"cyclesync",9},{"cyclespan",10},{"cyclestyle",0},{"morphtime",1200},{"mix",38},{"size",65},{"decay",4.5f},{"predelay",20} }, {} },
    { "Drift Between Worlds", "Morphing",
      { {"rtype",3},{"selmode",2},{"cyclerate",.03f},{"cyclespan",6},{"cyclestyle",2},{"mix",40},{"size",72},{"decay",6},{"predelay",36},{"width",130} }, {} },
    { "Tube Warm Hall", "Color",
      { {"rtype",3},{"selmode",0},{"mix",32},{"size",68},{"decay",3.4f},{"predelay",22},{"damping",7000},
        {"lowtype",2},{"midtype",2},{"hightype",1},{"lowdrive",10},{"middrive",12},{"highdrive",6},{"satmix",80},{"eqon",1},{"eqlow",2},{"eqhigh",-2} }, {} },
    { "Lo-Fi Crushed Verb", "Color",
      { {"rtype",4},{"selmode",0},{"mix",40},{"size",55},{"decay",3},{"predelay",18},{"lowcut",250},{"highcut",9000},
        {"lowtype",2},{"midtype",8},{"hightype",9},{"lowdrive",6},{"middrive",22},{"highdrive",18},{"satmix",75},
        {"eqon",1},{"eqfreq",1800},{"eqgain",4},{"eqq",1.2f} }, {} },
    { "Folded Shimmer", "Color",
      { {"rtype",9},{"selmode",0},{"mix",38},{"size",80},{"decay",8},{"predelay",30},{"hightype",5},{"highdrive",16},{"midtype",3},{"middrive",8},{"satmix",50},{"width",150} },
      { {3,"highdrive",0,25} } },
    { "Orbital Hall", "Motion",
      { {"rtype",3},{"selmode",0},{"mix",36},{"size",72},{"decay",4.2f},{"predelay",24},
        {"orbiton",1},{"orbitmode",0},{"orbitsync",8},{"around",85},{"updown",40},{"nearfar",55} }, {} },
    { "Auto Pan Plate", "Motion",
      { {"rtype",2},{"selmode",0},{"mix",32},{"size",45},{"decay",2.4f},{"predelay",12},
        {"panon",1},{"pansync",6},{"pandepth",80},{"panshape",0} }, {} },
    { "Gated 80s Snare", "Motion",
      { {"rtype",4},{"selmode",0},{"mix",38},{"size",60},{"decay",3.5f},{"predelay",4},{"damping",9000},
        {"gateon",1},{"gatemode",1},{"gatedepth",100},{"gatethresh",-30},{"gatehold",220},{"gatesmooth",8},
        {"midtype",4},{"middrive",10},{"satmix",50} }, {} },
    { "Trance Gate Verb", "Motion",
      { {"rtype",5},{"selmode",0},{"mix",45},{"size",75},{"decay",6},{"predelay",10},
        {"gateon",1},{"gatemode",0},{"gatesync",3},{"gatedepth",100},{"gateshape",4},{"gatesmooth",4} }, {} },
    { "Sidechain Pump Hall", "Pump",
      { {"rtype",3},{"selmode",0},{"mix",40},{"size",75},{"decay",5},{"predelay",20},
        {"duckmode",1},{"duckdepth",24},{"pumpsync",5},{"pumpcurve",2.5f} }, {} },
    { "Kick-Keyed Ambience", "Pump",
      { {"rtype",8},{"selmode",0},{"mix",45},{"size",85},{"decay",9},{"predelay",30},
        {"duckmode",0},{"ducksource",1},{"duckdepth",30},{"duckthresh",-28},{"duckattack",2},{"duckrelease",260} }, {} },
    { "Breathing Room", "Modulated",
      { {"rtype",1},{"selmode",1},{"mix",30},{"size",55},{"decay",2.2f},{"predelay",16} },
      { {1,"decay",0,-30},{1,"mix",0,-25},{3,"rtype",0,6} } },
    { "MIDI Swell", "Modulated",
      { {"rtype",5},{"selmode",1},{"mix",20},{"size",70},{"decay",6},{"predelay",24} },
      { {2,"mix",1,55},{2,"size",1,25} } },
    { "Freeze Pad", "Ambient",
      { {"rtype",9},{"selmode",0},{"mix",60},{"size",90},{"decay",20},{"predelay",40},{"width",170},{"moddepth",2.5f},
        {"orbiton",1},{"orbitmode",4},{"orbitrate",.05f},{"around",60},{"updown",30},{"nearfar",30} }, {} },
    };
    return p;
}
} // namespace velvet
