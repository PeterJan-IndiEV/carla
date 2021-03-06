// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "carla/client/World.h"

#include "carla/Logging.h"
#include "carla/client/Actor.h"
#include "carla/client/ActorBlueprint.h"
#include "carla/client/ActorList.h"
#include "carla/client/detail/Simulator.h"
#include "carla/StringUtil.h"

#include <exception>

namespace carla {
namespace client {

  SharedPtr<Map> World::GetMap() const {
    return _episode.Lock()->GetCurrentMap();
  }

  void World::LoadLevelLayer(rpc::MapLayer map_layers) const {
    _episode.Lock()->LoadLevelLayer(map_layers);
  }

  void World::UnloadLevelLayer(rpc::MapLayer map_layers) const {
    _episode.Lock()->UnloadLevelLayer(map_layers);
  }

  SharedPtr<BlueprintLibrary> World::GetBlueprintLibrary() const {
    return _episode.Lock()->GetBlueprintLibrary();
  }

  rpc::VehicleLightStateList World::GetVehiclesLightStates() const {
    return _episode.Lock()->GetVehiclesLightStates();
  }

  boost::optional<geom::Location> World::GetRandomLocationFromNavigation() const {
    return _episode.Lock()->GetRandomLocationFromNavigation();
  }

  SharedPtr<Actor> World::GetSpectator() const {
    return _episode.Lock()->GetSpectator();
  }

  rpc::EpisodeSettings World::GetSettings() const {
    return _episode.Lock()->GetEpisodeSettings();
  }

  uint64_t World::ApplySettings(const rpc::EpisodeSettings &settings) {
    return _episode.Lock()->SetEpisodeSettings(settings);
  }

  rpc::WeatherParameters World::GetWeather() const {
    return _episode.Lock()->GetWeatherParameters();
  }

  void World::SetWeather(const rpc::WeatherParameters &weather) {
    _episode.Lock()->SetWeatherParameters(weather);
  }

  WorldSnapshot World::GetSnapshot() const {
    return _episode.Lock()->GetWorldSnapshot();
  }

  SharedPtr<Actor> World::GetActor(ActorId id) const {
    auto simulator = _episode.Lock();
    auto description = simulator->GetActorById(id);
    return description.has_value() ?
        simulator->MakeActor(std::move(*description)) :
        nullptr;
  }

  SharedPtr<ActorList> World::GetActors() const {
    return SharedPtr<ActorList>{new ActorList{
                                  _episode,
                                  _episode.Lock()->GetAllTheActorsInTheEpisode()}};
  }

  SharedPtr<ActorList> World::GetActors(const std::vector<ActorId> &actor_ids) const {
    return SharedPtr<ActorList>{new ActorList{
                                  _episode,
                                  _episode.Lock()->GetActorsById(actor_ids)}};
  }

  SharedPtr<Actor> World::SpawnActor(
      const ActorBlueprint &blueprint,
      const geom::Transform &transform,
      Actor *parent_actor,
      rpc::AttachmentType attachment_type) {
    return _episode.Lock()->SpawnActor(blueprint, transform, parent_actor, attachment_type);
  }

  SharedPtr<Actor> World::TrySpawnActor(
      const ActorBlueprint &blueprint,
      const geom::Transform &transform,
      Actor *parent_actor,
      rpc::AttachmentType attachment_type) noexcept {
    try {
      return SpawnActor(blueprint, transform, parent_actor, attachment_type);
    } catch (const std::exception &) {
      return nullptr;
    }
  }

  WorldSnapshot World::WaitForTick(time_duration timeout) const {
    return _episode.Lock()->WaitForTick(timeout);
  }

  size_t World::OnTick(std::function<void(WorldSnapshot)> callback) {
    return _episode.Lock()->RegisterOnTickEvent(std::move(callback));
  }

  void World::RemoveOnTick(size_t callback_id) {
    _episode.Lock()->RemoveOnTickEvent(callback_id);
  }

  uint64_t World::Tick(time_duration timeout) {
    return _episode.Lock()->Tick(timeout);
  }

  void World::SetPedestriansCrossFactor(float percentage) {
    _episode.Lock()->SetPedestriansCrossFactor(percentage);
  }

  SharedPtr<Actor> World::GetTrafficSign(const Landmark& landmark) const {
    SharedPtr<ActorList> actors = GetActors();
    SharedPtr<TrafficSign> result;
    std::string landmark_id = landmark.GetId();
    for (size_t i = 0; i < actors->size(); i++) {
      SharedPtr<Actor> actor = actors->at(i);
      if (StringUtil::Match(actor->GetTypeId(), "*traffic.*")) {
        TrafficSign* sign = static_cast<TrafficSign*>(actor.get());
        if(sign && (sign->GetSignId() == landmark_id)) {
          return actor;
        }
      }
    }
    return nullptr;
  }

  SharedPtr<Actor> World::GetTrafficLight(const Landmark& landmark) const {
    SharedPtr<ActorList> actors = GetActors();
    SharedPtr<TrafficLight> result;
    std::string landmark_id = landmark.GetId();
    for (size_t i = 0; i < actors->size(); i++) {
      SharedPtr<Actor> actor = actors->at(i);
      if (StringUtil::Match(actor->GetTypeId(), "*traffic_light*")) {
        TrafficLight* tl = static_cast<TrafficLight*>(actor.get());
        if(tl && (tl->GetSignId() == landmark_id)) {
          return actor;
        }
      }
    }
    return nullptr;
  }

  void World::ResetAllTrafficLights() {
    _episode.Lock()->ResetAllTrafficLights();
  }

  SharedPtr<LightManager> World::GetLightManager() const {
    return _episode.Lock()->GetLightManager();
  }

  void World::FreezeAllTrafficLights(bool frozen) {
    _episode.Lock()->FreezeAllTrafficLights(frozen);
  }

  std::vector<geom::BoundingBox> World::GetLevelBBs(uint8_t queried_tag) const {
    return _episode.Lock()->GetLevelBBs(queried_tag);
  }

  std::vector<rpc::EnvironmentObject> World::GetEnvironmentObjects() const {
    return _episode.Lock()->GetEnvironmentObjects();
  }

  void World::EnableEnvironmentObjects(
      std::vector<uint64_t> env_objects_ids,
      bool enable) const {
    _episode.Lock()->EnableEnvironmentObjects(env_objects_ids, enable);
  }

  boost::optional<rpc::LabelledPoint> World::ProjectPoint(
      geom::Location location, geom::Vector3D direction, float search_distance) const {
    auto result = _episode.Lock()->ProjectPoint(location, direction, search_distance);
    if (result.first) {
      return result.second;
    }
    return {};
  }

  boost::optional<rpc::LabelledPoint> World::GroundProjection(
      geom::Location location, float search_distance) const {
    const geom::Vector3D DownVector(0,0,-1);
    return ProjectPoint(location, DownVector, search_distance);
  }

  std::vector<rpc::LabelledPoint> World::CastRay(
      geom::Location start_location, geom::Location end_location) const {
    return _episode.Lock()->CastRay(start_location, end_location);
  }

} // namespace client
} // namespace carla
