#pragma once

#include "bus.h"
#include "svg.h"
#include "transport_manager_command.h"
#include "stop.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

class MapBuilder {
public:
  MapBuilder(RenderSettings render_settings,
             const std::vector<Stop>& stops,
             const std::map<std::string, size_t>& stop_idx,
             const std::map<BusRoute::RouteNumber, BusRoute>& buses);

  std::string GetMap() const { return doc_.ToString(); }
  std::string GetRouteMap(const RouteInfo::Route &route) const { return BuildRoute(route).ToString(); }

private:
  RenderSettings render_settings_;

  const std::map<std::string, size_t>& stop_idx_;
  const std::map<BusRoute::RouteNumber, BusRoute>& buses_;
  std::map<std::string, Svg::Point> stop_coordinates_;
  std::map<std::string, Svg::Color> route_color;

  Svg::Document doc_;

  Svg::Point MapStop(const std::string& stop_name) const;

  Svg::Document BuildMap() const;

  Svg::Document BuildRoute(const RouteInfo::Route &route) const;
  void BuildTranslucentRoute(Svg::Document &doc) const;

  void BuildBusLines(Svg::Document &doc, const std::vector<std::pair<std::string, std::vector<std::string>>> &line) const;
  void BuildBusLines(Svg::Document& doc) const;
  void BuildBusLinesOnRoute(Svg::Document& doc, const RouteInfo::Route &route) const;

  void BuildBusLabels(Svg::Document &doc,
                      const std::vector<std::pair<std::string, std::vector<Svg::Point>>> &labels) const;
  void BuildBusLabels(Svg::Document& doc) const;
  void BuildBusLabelsOnRoute(Svg::Document& doc, const RouteInfo::Route &route) const;

  void BuildStopPoints(Svg::Document &doc, const std::vector<std::string> &stop_names) const;
  void BuildStopPoints(Svg::Document& doc) const;
  void BuildStopPointsOnRoute(Svg::Document& doc, const RouteInfo::Route &route) const;

  void BuildStopLabels(Svg::Document &doc, const std::vector<std::string> &stop_names) const;
  void BuildStopLabels(Svg::Document& doc) const;
  void BuildStopLabelsOnRoute(Svg::Document& doc, const RouteInfo::Route &route) const;

  static const std::unordered_map<MapLayer, void (MapBuilder::*)(Svg::Document&) const> build;
  static const std::unordered_map<
    MapLayer, void (MapBuilder::*)(Svg::Document&, const RouteInfo::Route &route) const> build_route;
};
