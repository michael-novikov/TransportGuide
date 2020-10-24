#include "svg.h"

namespace Svg {

void RenderColor(std::ostream& out, std::monostate) {
  out << "none";
}

void RenderColor(std::ostream& out, const Rgb& rgb) {
  out << "rgb(" << static_cast<int>(rgb.red)
      << "," << static_cast<int>(rgb.green)
      << "," << static_cast<int>(rgb.blue) << ")";
}

void RenderColor(std::ostream& out, const Rgba& rgba) {
  out << "rgba(" << static_cast<int>(rgba.red)
      << "," << static_cast<int>(rgba.green)
      << "," << static_cast<int>(rgba.blue)
      << "," << rgba.alpha << ")";
}

void RenderColor(std::ostream& out, const std::string& name) {
  out << name;
}

void RenderColor(std::ostream& out, const Color& color) {
  std::visit([&out](const auto& c) { RenderColor(out, c); }, color);
}

Circle& Circle::SetCenter(Point center) {
  center_ = center;
  return *this;
}

Circle& Circle::SetRadius(double r) {
  r_ = r;
  return *this;
}

void Circle::Render(std::ostream& out) const {
  out << "<circle ";

  ShapeProperties::RenderProperties(out);

  out << " " << "cx" << "=" << "\"" << center_.x << "\"";
  out << " " << "cy" << "=" << "\"" << center_.y << "\"";
  out << " " << "r" << "=" << "\"" << r_ << "\"";

  out << "/>";
}

Polyline& Polyline::AddPoint(Point point) {
  points_.push_back(point);
  return *this;
}

void Polyline::Render(std::ostream& out) const {
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

Text& Text::SetPoint(Point coordinates) {
  coordinates_ = coordinates;
  return *this;
}

Text& Text::SetOffset(Point offset) {
  offset_ = offset;
  return *this;
}

Text& Text::SetFontSize(uint32_t font_size) {
  font_size_ = font_size;
  return *this;
}

Text& Text::SetFontFamily(const std::string& font_family) {
  font_family_ = font_family;
  return *this;
}

Text& Text::SetData(const std::string& data) {
  data_ = data;
  return *this;
}

void Text::Render(std::ostream& out) const {
  out << "<text ";

  ShapeProperties::RenderProperties(out);

  out << " " << "x" << "=" << "\"" << coordinates_.x << "\"";
  out << " " << "y" << "=" << "\"" << coordinates_.y << "\"";

  out << " " << "dx" << "=" << "\"" << offset_.x << "\"";
  out << " " << "dy" << "=" << "\"" << offset_.y << "\"";

  out << " " << "font-size" << "=" << "\"" << font_size_ << "\"";

  if (font_family_) {
    out << " " << "font-family" << "=\"" << font_family_.value() << "\"";
  }

  out << ">";
  out << data_;
  out << "</text>";
}

void Document::Render(std::ostream& out) const {
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";

  for (const auto& shape : shapes_) {
    shape->Render(out);
  }

  out << "</svg>";
}

} // namespace Svg
