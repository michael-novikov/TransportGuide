#pragma once

#include "bus.h"
#include "svg.h"
#include "stop.h"
#include "transport_manager_command.h"

#include "bus.pb.h"
#include "router.pb.h"

#include <optional>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace TransportGuide {

class MapBuilder {
public:
  MapBuilder(const RenderSettings& render_settings,
             const StopDict& stop_dict,
             const BusDict& bus_dict,
             std::map<std::string, SvgPoint> coordinates_redefined,
             std::string map_string);

  MapBuilder(const RenderSettings& render_settings,
             const StopDict& stop_dict,
             const BusDict& bus_dict);

  std::string GetMapBody() const { return map_base_; }
  std::string GetMap() const { return Svg::Document::WrapBodyInSvg(map_base_); }
  std::string GetRouteMap(const RouteDescription::Route &route) const { return BuildRoute(route).ToStringBasedOn(map_base_); }

  static std::map<std::string, SvgPoint> ComputeStopsCoords(const RenderSettings& render_settings,
                                                            const StopDict& stop_dict,
                                                            const BusDict& buses);

private:
  const RenderSettings& render_settings_;
  const StopDict& stop_dict_;
  const BusDict& bus_dict_;

  std::map<std::string, SvgPoint> stop_coordinates_;
  std::map<std::string, Color> route_color;

  Svg::Document map_;
  std::string map_base_;

  SvgPoint MapStop(const std::string& stop_name) const;

  Svg::Document BuildMap() const;

  Svg::Document BuildRoute(const RouteDescription::Route &route) const;
  void BuildTranslucentRoute(Svg::Document &doc) const;

  void BuildBusLines(Svg::Document &doc, const std::vector<std::pair<std::string, std::vector<std::string>>> &line) const;
  void BuildBusLines(Svg::Document& doc) const;
  void BuildBusLinesOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const;

  void BuildBusLabels(Svg::Document &doc,
                      const std::vector<std::pair<std::string, std::vector<SvgPoint>>> &labels) const;
  void BuildBusLabels(Svg::Document& doc) const;
  void BuildBusLabelsOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const;

  void BuildStopPoints(Svg::Document &doc, const std::vector<std::string> &stop_names) const;
  void BuildStopPoints(Svg::Document& doc) const;
  void BuildStopPointsOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const;

  void BuildStopLabels(Svg::Document &doc, const std::vector<std::string> &stop_names) const;
  void BuildStopLabels(Svg::Document& doc) const;
  void BuildStopLabelsOnRoute(Svg::Document& doc, const RouteDescription::Route &route) const;

  static const std::unordered_map<
    std::string,
    void (MapBuilder::*)(Svg::Document&) const
  > build;
  static const std::unordered_map<
    std::string,
    void (MapBuilder::*)(Svg::Document&, const RouteDescription::Route &route) const
  > build_route;
};

}
