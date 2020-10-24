#pragma once

#include <cstdint>
#include <variant>
#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <optional>

namespace Svg {

struct Point {
  double x{0};
  double y{0};
};

struct Rgb {
  std::uint8_t red;
  std::uint8_t green;
  std::uint8_t blue;
};

using Color = std::variant<std::monostate, Rgb, std::string>;
const Color NoneColor{};

void RenderColor(std::ostream& out, std::monostate);
void RenderColor(std::ostream& out, const Rgb& rgb);
void RenderColor(std::ostream& out, const std::string& name);
void RenderColor(std::ostream& out, const Color& color);

class Shape {
public:
  virtual void Render(std::ostream& out) const = 0;
  virtual ~Shape() = default;
};

template <typename Owner>
class ShapeProperties {
public:
  Owner& SetFillColor(const Color& fill);
  Owner& SetStrokeColor(const Color& stroke);
  Owner& SetStrokeWidth(double stroke_width);
  Owner& SetStrokeLineCap(const std::string& stroke_linecap);
  Owner& SetStrokeLineJoin(const std::string& stroke_linejoin);

  Owner& AsOwner();

  void RenderProperties(std::ostream& out) const;

private:
  Color fill_{NoneColor};
  Color stroke_{NoneColor};
  double stroke_width_{1.0};
  std::optional<std::string> stroke_linecap_;
  std::optional<std::string> stroke_linejoin_;
};

template <typename Owner>
Owner& ShapeProperties<Owner>::SetFillColor(const Color& fill) {
  fill_ = fill;
  return AsOwner();
}

template <typename Owner>
Owner& ShapeProperties<Owner>::SetStrokeColor(const Color& stroke) {
  stroke_ = stroke;
  return AsOwner();
}

template <typename Owner>
Owner& ShapeProperties<Owner>::SetStrokeWidth(double stroke_width) {
  stroke_width_ = stroke_width;
  return AsOwner();
}

template <typename Owner>
Owner& ShapeProperties<Owner>::SetStrokeLineCap(const std::string& stroke_linecap) {
  stroke_linecap_ = stroke_linecap;
  return AsOwner();
}

template <typename Owner>
Owner& ShapeProperties<Owner>::SetStrokeLineJoin(const std::string& stroke_linejoin) {
  stroke_linejoin_ = stroke_linejoin;
  return AsOwner();
}

template <typename Owner>
Owner& ShapeProperties<Owner>::AsOwner() {
  return static_cast<Owner&>(*this);
}

template <typename Owner>
void ShapeProperties<Owner>::RenderProperties(std::ostream& out) const {
  out << "fill" << "=" << "\"";
  RenderColor(out, fill_);
  out << "\"";

  out << " " << "stroke" << "=" << "\"";
  RenderColor(out, stroke_);
  out << "\"";

  out << " " << "stroke-width" << "=" << "\"" << stroke_width_ << "\"";

  if (stroke_linecap_) {
    out << " " << "stroke-linecap" << "=" << "\"" << stroke_linecap_.value() << "\"";
  }

  if (stroke_linejoin_) {
    out << " " << "stroke-linejoin" << "=" << "\"" << stroke_linejoin_.value() << "\"";
  }
}

class Circle : public Shape, public ShapeProperties<Circle> {
public:
  Circle& SetCenter(Point center);
  Circle& SetRadius(double r);

  void Render(std::ostream& out) const;
private:
  Point center_{0.0, 0.0};
  double r_{1.0};
};

class Polyline : public Shape, public ShapeProperties<Polyline> {
public:
  Polyline& AddPoint(Point point);
  void Render(std::ostream& out) const;
private:
  std::vector<Point> points_;
};

class Text : public Shape, public ShapeProperties<Text> {
public:
  Text& SetPoint(Point coordinates);
  Text& SetOffset(Point offset);
  Text& SetFontSize(uint32_t font_size);
  Text& SetFontFamily(const std::string& font_family);
  Text& SetData(const std::string& data);

  void Render(std::ostream& out) const;
private:
  Point coordinates_;
  Point offset_;
  uint32_t font_size_;
  std::optional<std::string> font_family_;
  std::string data_;
};

class Document {
public:
  explicit Document() {}

  template <typename ShapeType>
  void Add(ShapeType shape);

  void Render(std::ostream& out) const;
private:
  std::vector<std::unique_ptr<Shape>> shapes_;
};

template <typename ShapeType>
void Document::Add(ShapeType shape) {
  shapes_.push_back(std::make_unique<ShapeType>(std::move(shape)));
}

} // namespace Svg
