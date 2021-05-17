#pragma once

#include "bus.h"
#include "svg.h"
#include "stop.h"
#include "transport_manager_command.h"

#include "bus.pb.h"
#include "router.pb.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace TransportGuide {

class MapBuilder {
public:
  MapBuilder(RenderSettings render_settings,
             const StopDict& stop_dict,
             const BusDict& buses);

  std::string GetMap() const { return map_.ToString(); }
  std::string GetRouteMap(const RouteDescription::Route &route) const { return BuildRoute(route).ToString(); }

private:
  RenderSettings render_settings_;

  const StopDict& stop_dict_;
  const BusDict& buses_;
  std::map<std::string, Svg::Point> stop_coordinates_;
  std::map<std::string, Svg::Color> route_color;

  Svg::Document map_;

  Svg::Point MapStop(const std::string& stop_name) const;

  Svg::Document BuildMap() const;

  Svg::Document BuildRoute(const RouteDescription::Route &route) const;
  void BuildTranslucentRoute(Svg::Document &doc) const;

  void BuildBusLines(Svg::Document &doc, const std::vector<std::pair<std::string, std::vector<std::string>>> &line) const;
  void BuildBusLines(Svg::Document& doc) const;
  void BuildBusLinesOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const;

  void BuildBusLabels(Svg::Document &doc,
                      const std::vector<std::pair<std::string, std::vector<Svg::Point>>> &labels) const;
  void BuildBusLabels(Svg::Document& doc) const;
  void BuildBusLabelsOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const;

  void BuildStopPoints(Svg::Document &doc, const std::vector<std::string> &stop_names) const;
  void BuildStopPoints(Svg::Document& doc) const;
  void BuildStopPointsOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const;

  void BuildStopLabels(Svg::Document &doc, const std::vector<std::string> &stop_names) const;
  void BuildStopLabels(Svg::Document& doc) const;
  void BuildStopLabelsOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const;

  static const std::unordered_map<MapLayer, void (MapBuilder::*)(Svg::Document&) const> build;
  static const std::unordered_map<
    MapLayer,
    void (MapBuilder::*)(Svg::Document&, const RouteDescription::Route &route) const
  > build_route;
};

}
