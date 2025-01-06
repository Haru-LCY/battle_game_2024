#pragma once
#include "battle_game/core/unit.h"

namespace battle_game::unit {
class ShikiTank : public Unit {
 public:
  ShikiTank(GameCore *game_core, uint32_t id, uint32_t player_id);
  void Render() override;
  void Update() override;
  [[nodiscard]] bool IsHit(glm::vec2 position) const override;

 protected:
  void TankMove(float move_speed, float rotate_angular_speed);
  void TurretRotate();
  void Fire();
  [[nodiscard]] const char *UnitName() const override;
  [[nodiscard]] const char *Author() const override;

  float turret_rotation_{0.0f};
  uint32_t fire_count_down_{0};
  uint32_t mine_count_down_{0};
  bool dashing{false};
  bool move_forward_{false};
  bool move_downward_{false};
  bool random_fire_{false};
};
}  // namespace battle_game::unit