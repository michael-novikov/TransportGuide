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
  double x, y;
};

struct Rgb {
  int red, green, blue;
};

void RenderColor(std::ostream& out, std::monostate) {
  out << "none";
}

void RenderColor(std::ostream& out, const Rgb& rgb) {
  out << "rgb(" << rgb.red << "," << rgb.green << "," << rgb.blue << ")";
}

void RenderColor(std::ostream& out, const std::string& name) {
  out << name;
}

using Color = std::variant<std::monostate, Rgb, std::string>;
const Color NoneColor{};

void RenderColor(std::ostream& out, const Color& color) {
  std::visit([&out](const Color& c) { RenderColor(out, c); }, color);
}

class Shape {
public:
  virtual Shape& SetFillColor(const Color& fill) { fill_ = fill; return *this; }
  virtual Shape& SetStrokeColor(const Color& stroke) { stroke_ = stroke; return *this; }
  virtual Shape& SetStrokeWidth(double stroke_width) { stroke_width_ = stroke_width; return *this; }
  virtual Shape& SetStrokeLineCap(const std::string& stroke_linecap) { stroke_linecap_ = stroke_linecap; return *this; }
  virtual Shape& SetStrokeLineJoin(const std::string& stroke_linejoin) { stroke_linejoin_ = stroke_linejoin; return *this; }

  virtual void Render(std::ostream& out) const = 0;

  virtual void RenderProperties(std::ostream& out) const {
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

protected:
  Color fill_{NoneColor};
  Color stroke_{NoneColor};
  double stroke_width_{1.0};
  std::string stroke_linecap_;
  std::string stroke_linejoin_;
};

class Circle : public Shape {
public:
  virtual Circle& SetFillColor(const Color& fill) override {
    Shape::SetFillColor(fill);
    return *this;
  }

  virtual Circle& SetStrokeColor(const Color& stroke) override {
    Shape::SetStrokeColor(stroke);
      return *this;
  }

  virtual Circle& SetStrokeWidth(double stroke_width) override {
    Shape::SetStrokeWidth(stroke_width);
    return *this;
  }

  virtual Circle& SetStrokeLineCap(const std::string& stroke_linecap) override {
    Shape::SetStrokeLineCap(stroke_linecap);
    return *this;
  }

  virtual Circle& SetStrokeLineJoin(const std::string& stroke_linejoin) override {
    Shape::SetStrokeLineJoin(stroke_linejoin);
    return *this;
  }

  Circle& SetCenter(Point center) { center_ = center; return *this; }
  Circle& SetRadius(double r) { r_ = r; return *this; }

  virtual void Render(std::ostream& out) const override {
    out << "<circle ";

    Shape::RenderProperties(out);

    out << " " << "cx" << "=" << "\"" << center_.x << "\"";
    out << " " << "cy" << "=" << "\"" << center_.y << "\"";
    out << " " << "r" << "=" << "\"" << r_ << "\"";

    out << "/>";
  }

private:
  Point center_{0.0, 0.0};
  double r_{1.0};
};

class Polyline : public Shape {
public:
  virtual Polyline& SetFillColor(const Color& fill) override {
    Shape::SetFillColor(fill);
    return *this;
  }

  virtual Polyline& SetStrokeColor(const Color& stroke) override {
    Shape::SetStrokeColor(stroke);
    return *this;
  }

  virtual Polyline& SetStrokeWidth(double stroke_width) override {
    Shape::SetStrokeWidth(stroke_width);
    return *this;
  }

  virtual Polyline& SetStrokeLineCap(const std::string& stroke_linecap) override {
    Shape::SetStrokeLineCap(stroke_linecap);
    return *this;
  }

  virtual Polyline& SetStrokeLineJoin(const std::string& stroke_linejoin) override {
    Shape::SetStrokeLineJoin(stroke_linejoin);
    return *this;
  }

  Polyline& AddPoint(Point point) { points_.push_back(point); return *this; }

  virtual void Render(std::ostream& out) const override {
    out << "<polyline ";

    Shape::RenderProperties(out);

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

class Text : public Shape {
public:
  virtual Text& SetFillColor(const Color& fill) override {
    Shape::SetFillColor(fill);
    return *this;
  }

  virtual Text& SetStrokeColor(const Color& stroke) override {
    Shape::SetStrokeColor(stroke);
      return *this;
  }

  virtual Text& SetStrokeWidth(double stroke_width) override {
    Shape::SetStrokeWidth(stroke_width);
    return *this;
  }

  virtual Text& SetStrokeLineCap(const std::string& stroke_linecap) override {
    Shape::SetStrokeLineCap(stroke_linecap);
    return *this;
  }

  virtual Text& SetStrokeLineJoin(const std::string& stroke_linejoin) override {
    Shape::SetStrokeLineJoin(stroke_linejoin);
    return *this;
  }

  Text& SetPoint(Point coordinates) { coordinates_ = coordinates; return *this; }
  Text& SetOffset(Point offset) { offset_ = offset; return *this; }
  Text& SetFontSize(uint32_t font_size) { font_size_ = font_size; return *this; }
  Text& SetFontFamily(const std::string& font_family) { font_family_ = font_family; return *this; }
  Text& SetData(const std::string& data) { data_ = data; return *this; }

  virtual void Render(std::ostream& out) const override {
    out << "<text ";

    Shape::RenderProperties(out);

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

  void Add(const Circle& shape) { shapes_.emplace_back(new Circle{shape}); }
  void Add(const Polyline& shape) { shapes_.emplace_back(new Polyline{shape}); }
  void Add(const Text& shape) { shapes_.emplace_back(new Text{shape}); }

  void Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";

    for (const auto& shape : shapes_) {
      shape->Render(out);
    }

    out << "</svg>";
  }

private:
  std::vector<std::shared_ptr<Shape>> shapes_;
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
