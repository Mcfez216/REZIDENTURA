// art.h - pictures made by the program itself: file photographs, covert street photographs, and the
// textures of paper, wood and worn ink. Everything is drawn from a seed, so the same person always
// has the same face. No photograph of any real person is used anywhere in the game.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

/** An image in memory: RGBA, one byte each, packed as ImGui and DirectX expect (0xAABBGGRR). */
struct Img {
    int w = 0, h = 0;
    std::vector<uint32_t> px;
};

namespace art {

enum Era { ERA_60S, ERA_70S, ERA_80S };

/** What a person looks like. Filled in from the game's data; the seed decides everything else. */
struct Face {
    uint32_t seed = 1;
    bool female = false;
    int age = 40;
    float skin = 0.74f;          // grey value of the skin in a black-and-white print
    bool uniform = false;        // a Soviet officer's file photograph, in uniform
    int era = ERA_60S;
};

/** A head-and-shoulders file photograph (3:4). */
Img portrait(const Face& f, int w, int h);

enum Scene { SC_MEET, SC_DROP, SC_SIGNAL, SC_ROUTE, SC_CANDID, SC_WATCHERS, SC_ARREST, SC_FLAT };
/** A covert photograph taken from a car or a window: grainy, long lens, somebody in the way. */
Img street(uint32_t seed, int scene, int era, int w, int h);

/** Tileable textures. */
Img paperGrain(int size, uint32_t seed);   // brown fibres and specks, mostly transparent
Img wood(int size, uint32_t seed);         // the desk
Img wear(int size, uint32_t seed);         // white specks: laid over a stamp, they read as missing ink

/** For the test tools. */
bool saveRaw(const Img& im, const std::string& path);

}  // namespace art
