#include "world_generator.h"

#include "logic/game.h"

#include "logic/components/city_component.h"
#include "logic/components/possession_component.h"
#include "logic/components/hexagon_component.h"
#include "logic/components/player_component.h"
#include "logic/components/capital_component.h"
#include "util/position_converter.h"

#include "PerlinNoise/PerlinNoise.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <random>

double WorldGenerator::distance(PositionComponent pc1, PositionComponent pc2) {
    auto pos1 = PositionConverter::hexToOrtho(pc1);
    auto pos2 = PositionConverter::hexToOrtho(pc2);
    return std::max(glm::distance2(pos1, pos2), 0.01f);
}

void WorldGenerator::generate(Game& game) {
    
    std::random_device r;
    std::knuth_b generator{r()};
    std::uniform_int_distribution<int> rand_distribution(0,100);
    auto rand = std::bind(rand_distribution, generator);

    std::uint32_t seed = r();
    const siv::PerlinNoise half_perlin(seed);

    
    for(uint y = 0; y < game.board.height; ++y) {
        for(uint x = 0; x < game.board.width; ++x) {

            double result = 0.0;
            double amp = 2.0;
            double freq = 2.5;

            glm::vec3 pos = PositionConverter::hexToOrtho(PositionComponent(x, y, 0));
            
            for (std::int32_t i = 0; i < 6; ++i)
            {
                result += half_perlin.noise(pos.x/game.board.width/3*freq, pos.z/game.board.height/3*freq) * amp;
                freq *= 1.5;
                amp *= 0.65;
            }
            
            float height = (result * 0.5 + 0.5)*4.f+0.5;

            if(height >= 1.5f) {
                game.board.set(x, y, game.factorySystem.createHexagon(x, y, height));
                if(x % 3 == 0 && y % 3 == 0 && rand() > 50/height) {
                    game.factorySystem.createCity(x,y,entt::null);
                }
            } else {
                game.board.set(x, y, game.factorySystem.createWater(x, y, 1.5));
            }
        }
    }

    
    //Permet d'espacer les capitales
    {
        std::vector<PositionComponent> player_pos(game.players.size());
        std::vector<double> player_weight(game.players.size());
        for(uint i = 0; i < game.players.size(); i++) {
            player_weight[i] = -10;
        }
        
        auto view = game.reg.view<CityComponent, PositionComponent>();
        
        const int numIter = 3;
        for(int t = 0; t<numIter; t++) {
        
            for(auto c: view) {
                
                double bestDistance = -1;
                int bestIndex = -1;

                auto cpos = view.get<PositionComponent>(c);
                
                for(uint i = 0; i < game.players.size(); ++i) {
                    double minDistance = 999999;
                    for(uint j = 0; j < game.players.size(); ++j) {
                        if(j != i) {
                            double dist = distance(cpos, player_pos[j]);
                            if(dist < minDistance) {
                                minDistance = dist;
                            }
                        }
                    }
                    
                    if(minDistance > bestDistance && minDistance > player_weight[i]) {
                        bestDistance = minDistance;
                        bestIndex = i;
                    }
                }

                if(bestIndex != -1) {
                    player_weight[bestIndex] = bestDistance;
                    player_pos[bestIndex] = cpos;
                }
                
            }
            
        }

        for(uint i = 0; i < game.players.size(); ++i) {
            Entity c = game.reg.get<HexagonComponent>(game.board.get(player_pos[i])).structure;
            game.reg.assign<CapitalComponent>(c);
            if(game.reg.has<PossessionComponent>(c)) game.reg.remove<PossessionComponent>(c);
            game.reg.assign<PossessionComponent>(c, game.players[i]);
            auto& playComp = game.reg.get<PlayerComponent>(game.players[i]);
            playComp.capital = c;
            playComp.cameraPos = PositionConverter::hexToOrtho(player_pos[i]);
        }
    }
    
}

