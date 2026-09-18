#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>

// Presentation-only observation. This module never receives mutable Game state.
namespace dw::visual
{
    enum class Motion { Idle, Walk, Attack, Hurt, Down };
    struct Sample
    {
        float x = 0, y = 0, hp = 100, cooldown = 0;
        float attack = 0; // normalized progress of an existing attack
        bool attacking = false, windup = false, locked = false;
        float rate = 1;
        int initialFacing = 1;
    };
    struct Pose
    {
        Motion motion = Motion::Idle;
        float phase = 0;
        int facing = 1;
        float recoil = 0;
        bool flash = false;
    };
    struct Track
    {
        Sample previous;
        bool initialized = false;
        float lastTime = 0, movedAt = -100, hitAt = -100, firedAt = -100;
        float stride = 0, speed = 0;
        int facing = 1;
        std::uint64_t seen = 0;
    };

    class Animator
    {
        std::unordered_map<std::uint64_t, Track> tracks;
        float previousAge = -1, time = 0;
        int scene = -1;
        std::uint64_t frame = 0;
    public:
        void begin(float gameAge, int sceneId, bool frozen)
        {
            if (scene != sceneId || gameAge < previousAge - .001f)
            {
                tracks.clear(); time = 0; previousAge = gameAge; scene = sceneId;
            }
            // No wall clock: pause, repeated draws and identical network snapshots freeze.
            if (!frozen && previousAge >= 0)
                time += std::clamp(gameAge - previousAge, 0.f, .1f);
            previousAge = gameAge;
            ++frame;
            for (auto i = tracks.begin(); i != tracks.end();)
                if (frame > i->second.seen + 2) i = tracks.erase(i); else ++i;
        }
        Pose observe(std::uint64_t key, const Sample& s)
        {
            auto& t = tracks[key];
            if (!t.initialized)
            {
                t.previous = s; t.lastTime = time; t.facing = s.initialFacing;
                t.initialized = true;
            }
            const float dt = time - t.lastTime;
            const float dx = s.x - t.previous.x, dy = s.y - t.previous.y;
            const float distance = std::sqrt(dx * dx + dy * dy);
            const bool discontinuity = distance > 48.f || s.hp > t.previous.hp + 30.f;
            if (discontinuity)
            {
                t.movedAt = t.hitAt = t.firedAt = -100;
                t.speed = t.stride = 0;
            }
            else if (dt > 0)
            {
                if (s.hp < t.previous.hp - .05f && time - t.hitAt > .16f)
                    t.hitAt = time;
                // A cooldown increase represents an actual attack already performed.
                if (s.cooldown > t.previous.cooldown + .15f)
                    t.firedAt = time;
                if (distance > .025f && !s.locked)
                {
                    t.movedAt = time;
                    t.speed = std::clamp(distance / dt, 8.f, 150.f);
                    if (std::abs(dx) > .05f) t.facing = dx < 0 ? -1 : 1;
                }
                if (time - t.movedAt < .10f && !s.locked)
                    t.stride += dt * std::clamp(t.speed / 20.f, 1.5f, 6.f) * s.rate;
            }
            t.seen = frame;
            t.previous = s; t.lastTime = time;
            Pose p; p.facing = t.facing;
            if (s.hp <= 0) p.motion = Motion::Down;
            else if (time - t.hitAt < .22f)
            {
                p.motion = Motion::Hurt; p.phase = (time - t.hitAt) / .22f;
                p.recoil = (1 - p.phase) * 2;
                p.flash = p.phase < .32f || (p.phase > .55f && p.phase < .72f);
            }
            else if (s.attacking || s.windup || time - t.firedAt < .24f)
            {
                p.motion = Motion::Attack;
                p.phase = s.attacking ? std::clamp(s.attack, 0.f, 1.f) :
                    s.windup ? .10f : std::clamp((time - t.firedAt) / .24f, 0.f, 1.f);
            }
            else if (time - t.movedAt < .10f && !s.locked)
            {
                p.motion = Motion::Walk; p.phase = t.stride - std::floor(t.stride);
            }
            else
            {
                p.phase = std::fmod(time * .65f * s.rate + (key % 7) * .13f, 1.f);
            }
            return p;
        }
        std::size_t size() const { return tracks.size(); }
    };

    enum class Rig { Humanoid, Beast, Floating, Boss };
    struct Offset { float x = 0, y = 0; };
    inline float smooth(float a, float b, float x)
    {
        x = std::clamp((x-a)/(b-a), 0.f, 1.f); return x*x*(3-2*x);
    }
    // Eight authored, discrete poses per cycle; integer offsets preserve the pixel grid.
    inline Offset deform(float u, float v, const Pose& p, Rig rig)
    {
        static constexpr float step[] = {0, .75f, 1, .75f, 0, -.75f, -1, -.75f};
        const int frame = std::clamp(int(p.phase * 8), 0, 7);
        const float wave = step[frame];
        const float side = u < .5f ? -1.f : 1.f;
        const float legs = smooth(.66f, .86f, v);
        const float upper = 1 - smooth(.55f, .94f, v);
        const float arms = smooth(.55f, .9f, std::abs(u-.5f)*2) *
            smooth(.30f, .48f, v) * (1-smooth(.73f,.91f,v));
        Offset o;
        if (p.motion == Motion::Idle)
        {
            o.y = -std::max(0.f, wave) * upper;
            if (rig == Rig::Floating) o.y -= wave;
            if (rig == Rig::Boss) o.x = wave * (u-.5f) * upper;
        }
        else if (p.motion == Motion::Walk)
        {
            o.y = -std::abs(wave) * upper;
            if (rig == Rig::Humanoid || rig == Rig::Beast)
            {
                o.x = wave * side * legs * 1.5f - wave * side * arms;
                o.y -= std::max(0.f, wave * side) * legs * 2.f;
                if (rig == Rig::Beast) o.x += wave * (1-v) * 1.5f;
            }
            else
            {
                o.y -= wave * .8f;
                o.x += wave * (u-.5f) * (rig == Rig::Boss ? 1.f : 2.f);
            }
        }
        else if (p.motion == Motion::Attack)
        {
            static constexpr float reach[] = {-1,-1,1,3,3,2,1,0};
            const float a = reach[frame];
            o.x = a * upper * .55f + a * arms;
            o.y = -std::max(0.f,a) * arms * .5f;
            if (rig == Rig::Humanoid)
            {
                static constexpr float swing[] = {-.18f,-.32f,0,.42f,.58f,.38f,.14f,0};
                const float angle=swing[frame];
                const float dx=(u-.64f)*32.f, dy=(v-.52f)*38.f;
                const float weight=smooth(.58f,.82f,u) * smooth(.27f,.43f,v) *
                    (1-smooth(.78f,.95f,v));
                o.x += ((std::cos(angle)-1)*dx-std::sin(angle)*dy)*weight;
                o.y += (std::sin(angle)*dx+(std::cos(angle)-1)*dy)*weight;
            }
            if (rig == Rig::Beast || rig == Rig::Floating) o.x = a * upper;
            if (rig == Rig::Boss) { o.x = a * upper * .65f; o.y = -a * .35f * upper; }
        }
        else if (p.motion == Motion::Hurt)
        {
            o.x = -p.recoil * upper;
            o.y = upper * (frame < 3 ? 1.f : 0.f);
        }
        return {std::round(o.x), std::round(o.y)};
    }
}
