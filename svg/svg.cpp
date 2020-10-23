#include <iostream>
#include <optional>
#include <ostream>
#include <string>
#include <map>
#include <type_traits>
#include <vector>
#include <cstdint>
#include <utility>
#include <variant>
#include <memory>

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

void RenderColor(std::ostream& out, std::monostate) {
  out << "none";
}

void RenderColor(std::ostream& out, const Rgb& rgb) {
  out << "rgb(" << static_cast<int>(rgb.red)
      << "," << static_cast<int>(rgb.green)
      << "," << static_cast<int>(rgb.blue) << ")";
}

void RenderColor(std::ostream& out, const std::string& name) {
  out << name;
}

void RenderColor(std::ostream& out, const Color& color) {
  std::visit([&out](const auto& c) { RenderColor(out, c); }, color);
}

class Shape {
public:
  virtual void Render(std::ostream& out) const = 0;
  virtual ~Shape() = default;
};

template <typename Owner>
class ShapeProperties {
public:
  Owner& SetFillColor(const Color& fill) { fill_ = fill; return AsOwner(); }
  Owner& SetStrokeColor(const Color& stroke) { stroke_ = stroke; return AsOwner(); }
  Owner& SetStrokeWidth(double stroke_width) { stroke_width_ = stroke_width; return AsOwner(); }
  Owner& SetStrokeLineCap(const std::string& stroke_linecap) { stroke_linecap_ = stroke_linecap; return AsOwner(); }
  Owner& SetStrokeLineJoin(const std::string& stroke_linejoin) { stroke_linejoin_ = stroke_linejoin; return AsOwner(); }

  Owner& AsOwner() { return static_cast<Owner&>(*this); }

  void RenderProperties(std::ostream& out) const {
    out << "fill" << "=" << "\"";
    RenderColor(out, fill_);
    out << "\"";

    out << " " << "stroke" << "=" << "\"";
    RenderColor(out, stroke_);
    out << "\"";

    out << " " << "stroke-width" << "=" << "\"" << stroke_width_ << "\"";

    if (stroke_linecap_ != "") {
      out << " " << "stroke-linecap" << "=" << "\"" << stroke_linecap_ << "\"";
    }

    if (stroke_linejoin_ != "") {
      out << " " << "stroke-linejoin" << "=" << "\"" << stroke_linejoin_ << "\"";
    }
  }

private:
  Color fill_{NoneColor};
  Color stroke_{NoneColor};
  double stroke_width_{1.0};
  std::string stroke_linecap_;
  std::string stroke_linejoin_;
};

class Circle : public Shape, public ShapeProperties<Circle> {
public:
  Circle& SetCenter(Point center) { center_ = center; return *this; }
  Circle& SetRadius(double r) { r_ = r; return *this; }

  void Render(std::ostream& out) const {
    out << "<circle ";

    ShapeProperties::RenderProperties(out);

    out << " " << "cx" << "=" << "\"" << center_.x << "\"";
    out << " " << "cy" << "=" << "\"" << center_.y << "\"";
    out << " " << "r" << "=" << "\"" << r_ << "\"";

    out << "/>";
  }

private:
  Point center_{0.0, 0.0};
  double r_{1.0};
};

class Polyline : public Shape, public ShapeProperties<Polyline> {
public:
  Polyline& AddPoint(Point point) { points_.push_back(point); return *this; }

  void Render(std::ostream& out) const {
    out << "<polyline ";

    ShapeProperties::RenderProperties(out);

    out << " " << "points" << "=" << "\"";

    bool first = true;
    for (const auto& point : points_) {
      if (first) {
        first = false;
      } else {
        out << " ";
      }

      out << point.x << "," << point.y;
    }

    out << "\"";

    out << "/>";
  }

private:
  std::vector<Point> points_;
};

class Text : public Shape, public ShapeProperties<Text> {
public:
  Text& SetPoint(Point coordinates) { coordinates_ = coordinates; return *this; }
  Text& SetOffset(Point offset) { offset_ = offset; return *this; }
  Text& SetFontSize(uint32_t font_size) { font_size_ = font_size; return *this; }
  Text& SetFontFamily(const std::string& font_family) { font_family_ = font_family; return *this; }
  Text& SetData(const std::string& data) { data_ = data; return *this; }

  void Render(std::ostream& out) const {
    out << "<text ";

    ShapeProperties::RenderProperties(out);

    out << " " << "x" << "=" << "\"" << coordinates_.x << "\"";
    out << " " << "y" << "=" << "\"" << coordinates_.y << "\"";

    out << " " << "dx" << "=" << "\"" << offset_.x << "\"";
    out << " " << "dy" << "=" << "\"" << offset_.y << "\"";

    out << " " << "font-size" << "=" << "\"" << font_size_ << "\"";

    if (font_family_ != "") {
      out << " " << "font-family" << "=\"" << font_family_ << "\"";
    }

    out << ">";
    out << data_;
    out << "</text>";
  }

private:
  Point coordinates_;
  Point offset_;
  uint32_t font_size_;
  std::string font_family_;
  std::string data_;
};

class Document {
public:
  explicit Document() {}

  template <typename ShapeType>
  void Add(ShapeType shape) { shapes_.push_back(std::make_unique<ShapeType>(std::move(shape))); }

  void Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";

    for (const auto& shape : shapes_) {
      shape->Render(out);
    }

    out << "</svg>";
  }

private:
  std::vector<std::unique_ptr<Shape>> shapes_;
};

}

int main() {
  Svg::Document svg;

  svg.Add(
      Svg::Polyline{}
      .SetStrokeColor(Svg::Rgb{140, 198, 63})  // soft green
      .SetStrokeWidth(16)
      .SetStrokeLineCap("round")
      .AddPoint({50, 50})
      .AddPoint({250, 250})
  );

  for (const auto point : {Svg::Point{50, 50}, Svg::Point{250, 250}}) {
    svg.Add(
        Svg::Circle{}
        .SetFillColor("white")
        .SetRadius(6)
        .SetCenter(point)
    );
  }

  svg.Add(
      Svg::Text{}
      .SetPoint({50, 50})
      .SetOffset({10, -10})
      .SetFontSize(20)
      .SetFontFamily("Verdana")
      .SetFillColor("black")
      .SetData("C")
  );
  svg.Add(
      Svg::Text{}
      .SetPoint({250, 250})
      .SetOffset({10, -10})
      .SetFontSize(20)
      .SetFontFamily("Verdana")
      .SetFillColor("black")
      .SetData("C++")
  );

  svg.Render(std::cout);
}
