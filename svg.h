#pragma once

#include "router.pb.h"

#include <cstdint>
#include <variant>
#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <optional>

namespace TransportGuide {
namespace Svg {

const Color NoneColor;

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
  Owner& SetOuterMargin(double outer_margin);

  Owner& AsOwner();

  void RenderProperties(std::ostream& out) const;

private:
  Color fill_;
  Color stroke_;
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

class Rectangle : public Shape, public ShapeProperties<Rectangle> {
public:
  Rectangle() {
    base_point_.set_x(0);
    base_point_.set_y(0);
  }

  Rectangle& SetBasePoint(const SvgPoint& p);
  Rectangle& SetWidth(double w);
  Rectangle& SetHeight(double h);

  void Render(std::ostream& out) const;
private:
  SvgPoint base_point_;
  double w_{1.0};
  double h_{1.0};
};

class Circle : public Shape, public ShapeProperties<Circle> {
public:
  Circle() {
    center_.set_x(0);
    center_.set_y(0);
  }

  Circle& SetCenter(const SvgPoint& center);
  Circle& SetRadius(double r);

  void Render(std::ostream& out) const;
private:
  SvgPoint center_;
  double r_{1.0};
};

class Polyline : public Shape, public ShapeProperties<Polyline> {
public:
  Polyline& AddPoint(const SvgPoint& point);
  void Render(std::ostream& out) const;
private:
  std::vector<SvgPoint> points_;
};

class Text : public Shape, public ShapeProperties<Text> {
public:
  Text& SetPoint(const SvgPoint& coordinates);
  Text& SetOffset(const SvgPoint& offset);
  Text& SetFontSize(uint32_t font_size);
  Text& SetFontFamily(const std::string& font_family);
  Text& SetFontWeight(const std::string& font_weight);
  Text& SetData(const std::string& data);

  void Render(std::ostream& out) const;
private:
  SvgPoint coordinates_;
  SvgPoint offset_;
  uint32_t font_size_;
  std::optional<std::string> font_family_;
  std::optional<std::string> font_weight_;
  std::string data_;
};

class Document {
public:
  explicit Document() {}

  template <typename ShapeType>
  void Add(ShapeType shape);

  void Render(std::ostream& out) const;
  std::string ToString() const;
  static std::string WrapBodyInSvg(const std::string& body);
  std::string BodyToString() const;
  std::string ToStringBasedOn(const std::string& base) const;

private:
  std::vector<std::unique_ptr<Shape>> shapes_;
};

template <typename ShapeType>
void Document::Add(ShapeType shape) {
  shapes_.push_back(std::make_unique<ShapeType>(std::move(shape)));
}

} // namespace Svg
} // namespace TransportGuide
