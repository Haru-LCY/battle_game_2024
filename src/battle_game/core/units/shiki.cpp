#include "shiki.h"
#include "battle_game/core/bullets/bullets.h"
#include "battle_game/core/game_core.h"
#include "battle_game/graphics/graphics.h"
#include"battle_game/core/particles/smoke.h"
#include<random>
namespace battle_game::unit {

namespace {
uint32_t tank_body_model_index = 0xffffffffu;
uint32_t tank_turret_model_index = 0xffffffffu;
}  // namespace
ShikiTank::ShikiTank(GameCore *game_core, uint32_t id, uint32_t player_id)
    : Unit(game_core, id, player_id) {
  if (!~tank_body_model_index) {
    auto mgr = AssetsManager::GetInstance();
    {
      /* Tank Body */
      tank_body_model_index = mgr->RegisterModel(
          {
              {{-0.8f, 0.8f}, {0.0f, 0.1f}, {0.9f, 1.0f, 1.0f, 1.0f}},
              {{-0.8f, -1.0f}, {0.0f, 0.2f}, {0.8f, 1.0f, 1.0f, 1.0f}},
              {{0.8f, 0.8f}, {0.0f, 0.3f}, {1.0f, 0.7f, 1.0f, 1.0f}},
              {{0.8f, -1.0f}, {0.0f, 0.4f}, {1.2f, 1.0f, 1.0f, 1.0f}},
              // distinguish front and back
              {{0.6f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.7f, 1.0f, 1.0f}},
              {{-0.6f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
          },
          {0, 1, 2, 1, 2, 3, 0, 2, 5, 2, 4, 5});
    }

    {
      /* Tank Turret */
      std::vector<ObjectVertex> turret_vertices;
      std::vector<uint32_t> turret_indices;
      const int precision = 60;
      const float inv_precision = 1.0f / float(precision);
      for (int i = 0; i < precision; i++) {
        auto theta = (float(i) + 0.5f) * inv_precision;
        theta *= glm::pi<float>() * 2.0f;
        auto sin_theta = std::sin(theta);
        auto cos_theta = std::cos(theta);
        turret_vertices.push_back({{sin_theta * 0.5f, cos_theta * 0.5f},
                                   {0.0f, 0.0f},
                                   {0.7f, 0.7f, 0.7f, 1.0f}});
        turret_indices.push_back(i);
        turret_indices.push_back((i + 1) % precision);
        turret_indices.push_back(precision);
      }
      turret_vertices.push_back(
          {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{-0.1f, 0.1f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{0.1f, 0.3f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{-0.1f, 1.2f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_vertices.push_back(
          {{0.1f, 1.5f}, {0.0f, 0.0f}, {0.7f, 0.7f, 0.7f, 1.0f}});
      turret_indices.push_back(precision + 1 + 0);
      turret_indices.push_back(precision + 1 + 1);
      turret_indices.push_back(precision + 1 + 2);
      turret_indices.push_back(precision + 1 + 1);
      turret_indices.push_back(precision + 1 + 2);
      turret_indices.push_back(precision + 1 + 3);
      tank_turret_model_index =
          mgr->RegisterModel(turret_vertices, turret_indices);
    }
  }
}

void ShikiTank::Render() {
  battle_game::SetTransformation(position_, rotation_);
  battle_game::SetTexture(0);
  battle_game::SetColor(game_core_->GetPlayerColor(player_id_));
  battle_game::DrawModel(tank_body_model_index);
  battle_game::SetRotation(turret_rotation_);
  battle_game::DrawModel(tank_turret_model_index);
}

void ShikiTank::Update() {
  TankMove(3.0f, glm::radians(180.0f));
  TurretRotate();
  Fire();
}

void ShikiTank::TankMove(float move_speed, float rotate_angular_speed) {
  auto player = game_core_->GetPlayer(player_id_);

  if (player) {
    auto &input_data = player->GetInputData();
    glm::vec2 offset{0.0f};
    float speed = move_speed * GetSpeedScale();
    dashing = false;

    if (input_data.mouse_button_clicked[GLFW_MOUSE_BUTTON_RIGHT]) {
      move_forward_ = true;
    }
    if (input_data.mouse_button_clicked[GLFW_MOUSE_BUTTON_LEFT]) {
      move_forward_ = false;
    }

    if(move_forward_)
    {
      offset.x += 1.0f;
    }
    else
    {
      offset.x -= 1.0f;
    }
    //按下2向下移动
      if (input_data.key_down[GLFW_KEY_2]) {
        move_downward_ = true;
      }
    if (input_data.key_down[GLFW_KEY_3]) {
      move_downward_ = false;
    }
    if(move_downward_)
    {
      offset.y -= 1.0f;
    }
    else
    {
      offset.y += 1.0f;
    }

    //检测是否按下1进入冲刺模式
    if (input_data.key_down[GLFW_KEY_1]) {
      speed = 6.0f * GetSpeedScale();
      dashing = true;
    }

    offset *= kSecondPerTick * speed;
    auto new_position =
        position_ + glm::vec2{glm::rotate(glm::mat4{1.0f}, rotation_,
                                          glm::vec3{0.0f, 0.0f, 1.0f}) *
                              glm::vec4{offset, 0.0f, 0.0f}};
    if (!game_core_->IsBlockedByObstacles(new_position)) {
      game_core_->PushEventMoveUnit(id_, new_position);
    }
    
    // 检测障碍物并反弹
    if (game_core_->IsBlockedByObstacles(new_position)) {
      if (game_core_->IsBlockedByObstacles(position_ + glm::vec2{offset.x, 0.0f})) {
       move_forward_ = !move_forward_;
      }
      if (game_core_->IsBlockedByObstacles(position_ + glm::vec2{0.0f, offset.y})) {
        move_downward_ = !move_downward_;
      }
      new_position = position_ + offset;
    }

    if (!game_core_->IsBlockedByObstacles(new_position)) {
      game_core_->PushEventMoveUnit(id_, new_position);
    }

    if(!dashing)//dashing prevents rotation
    {
    float rotation_offset = 0.0f;
    if (input_data.key_down[GLFW_KEY_A]) {
      rotation_offset += 1.0f;
    }
    if (input_data.key_down[GLFW_KEY_D]) {
      rotation_offset -= 1.0f;
    }
    rotation_offset *= kSecondPerTick * rotate_angular_speed * GetSpeedScale();
    game_core_->PushEventRotateUnit(id_, rotation_ + rotation_offset);
    }
  }
}

void ShikiTank::TurretRotate() {
  auto player = game_core_->GetPlayer(player_id_);
  if (player) {
    auto &input_data = player->GetInputData();
    auto diff = input_data.mouse_cursor_position - position_;
    if (glm::length(diff) < 1e-4) {
      turret_rotation_ = rotation_;
    } else {
      turret_rotation_ = std::atan2(diff.y, diff.x) - glm::radians(90.0f);
    }
  }
}

void ShikiTank::Fire() {
  if (fire_count_down_ == 0 && !dashing) {  // 只有在不冲刺时才能开火
    auto player = game_core_->GetPlayer(player_id_);
    if (player) {
      if(player->GetInputData().key_down[GLFW_KEY_4])
      {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(0.0, 2.0 * glm::pi<float>());

      // 生成10个随机角度的子弹
      for (int i = 0; i < 30; ++i) {
        float random_angle = dis(gen);
        glm::vec2 bullet_position = position_ + glm::vec2(glm::cos(random_angle), glm::sin(random_angle)) * 1.2f;
        glm::vec2 velocity = Rotate(glm::vec2{0.0f, 20.0f}, random_angle);

        GenerateBullet<bullet::CannonBall>(
            bullet_position,
            random_angle, GetDamageScale(), velocity);
      }
      }
      else
      {auto velocity = Rotate(glm::vec2{0.0f, 20.0f}, turret_rotation_);
        GenerateBullet<bullet::CannonBall>(
            position_ + Rotate({0.0f, 1.2f}, turret_rotation_),
            turret_rotation_, GetDamageScale(), velocity);
        }
        for (int i = 0; i < 5; ++i) {
        glm::vec2 smoke_position = position_ + Rotate({0.0f, 1.2f}, turret_rotation_);
        glm::vec2 smoke_velocity = Rotate(glm::vec2{0.0f, 5.0f}, turret_rotation_);
        game_core_->PushEventGenerateParticle<particle::Smoke>(
            smoke_position, rotation_, smoke_velocity, 1.0f, glm::vec4{0.5f, 0.5f, 0.5f, 1.0f}, 0.5f);
      }
      fire_count_down_ = kTickPerSecond;  // Fire interval 1 second.
    }
  }
  if (fire_count_down_) {
    fire_count_down_--;
  }
}


bool ShikiTank::IsHit(glm::vec2 position) const {
  position = WorldToLocal(position);
  return position.x > -0.8f && position.x < 0.8f && position.y > -1.0f &&
         position.y < 1.0f && position.x + position.y < 1.6f &&
         position.y - position.x < 1.6f;
}

const char *ShikiTank::UnitName() const {
  return "ShikiTank";
}

const char *ShikiTank::Author() const {
  return "Shiki";
}
}  // namespace battle_game::unit
