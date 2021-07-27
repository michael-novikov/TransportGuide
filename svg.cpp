#include "svg.h"

#include <sstream>

namespace TransportGuide {
namespace Svg {

void RenderColor(std::ostream& out, const Color& color) {
  if (color.has_rgb()) {
    auto rgb = color.rgb();
    out << "rgb(" << static_cast<int>(rgb.red())
        << "," << static_cast<int>(rgb.green())
        << "," << static_cast<int>(rgb.blue()) << ")";
  }
  else if (color.has_rgba()) {
    auto rgba = color.rgba();
    out << "rgba(" << static_cast<int>(rgba.red())
        << "," << static_cast<int>(rgba.green())
        << "," << static_cast<int>(rgba.blue())
        << "," << rgba.alpha() << ")";
  }
  else if (color.name().size() > 0) {
    out << color.name();
  }
  else {
    out << "none";
  }
}

Rectangle& Rectangle::SetBasePoint(const SvgPoint& p) {
  base_point_.CopyFrom(p);
  return *this;
}

Rectangle& Rectangle::SetWidth(double w) {
  w_ = w;
  return *this;
}

Rectangle& Rectangle::SetHeight(double h) {
  h_ = h;
  return *this;
}

void Rectangle::Render(std::ostream& out) const {
  out << "<rect";

  out << " " << "x" << "=" << "\"" << base_point_.x() << "\"";
  out << " " << "y" << "=" << "\"" << base_point_.y() << "\"";
  out << " " << "width" << "=" << "\"" << w_ << "\"";
  out << " " << "height" << "=" << "\"" << h_ << "\"";

  out << " ";
  ShapeProperties::RenderProperties(out);
  out << " />";
}

Circle& Circle::SetCenter(const SvgPoint& center) {
  center_.CopyFrom(center);
  return *this;
}

Circle& Circle::SetRadius(double r) {
  r_ = r;
  return *this;
}

void Circle::Render(std::ostream& out) const {
  out << "<circle ";

  ShapeProperties::RenderProperties(out);

  out << " " << "cx" << "=" << "\"" << center_.x() << "\"";
  out << " " << "cy" << "=" << "\"" << center_.y() << "\"";
  out << " " << "r" << "=" << "\"" << r_ << "\"";

  out << "/>";
}

Polyline& Polyline::AddPoint(const SvgPoint& point) {
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

    out << point.x() << "," << point.y();
  }

  out << "\"";

  out << "/>";
}

Text& Text::SetPoint(const SvgPoint& coordinates) {
  coordinates_.CopyFrom(coordinates);
  return *this;
}

Text& Text::SetOffset(const SvgPoint& offset) {
  offset_.CopyFrom(offset);
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

Text& Text::SetFontWeight(const std::string& font_weight) {
  font_weight_ = font_weight;
  return *this;
}

Text& Text::SetData(const std::string& data) {
  data_ = data;
  return *this;
}

void Text::Render(std::ostream& out) const {
  out << "<text ";

  ShapeProperties::RenderProperties(out);

  out << " " << "x" << "=" << "\"" << coordinates_.x() << "\"";
  out << " " << "y" << "=" << "\"" << coordinates_.y() << "\"";

  out << " " << "dx" << "=" << "\"" << offset_.x() << "\"";
  out << " " << "dy" << "=" << "\"" << offset_.y() << "\"";

  out << " " << "font-size" << "=" << "\"" << font_size_ << "\"";

  if (font_family_) {
    out << " " << "font-family" << "=\"" << font_family_.value() << "\"";
  }

  if (font_weight_) {
    out << " " << "font-weight" << "=\"" << font_weight_.value() << "\"";
  }

  out << ">";
  out << data_;
  out << "</text>";
}

void Document::Render(std::ostream& out) const {
  for (const auto& shape : shapes_) {
    shape->Render(out);
  }
}

std::string Document::ToString() const {
  return WrapBodyInSvg(BodyToString());
}

std::string Document::WrapBodyInSvg(const std::string& body) {
  std::ostringstream out;

  out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">";
  out << body;
  out << "</svg>";

  return out.str();
}

std::string Document::BodyToString() const {
  std::ostringstream out;
  Render(out);
  return out.str();
}

std::string Document::ToStringBasedOn(const std::string& base) const {
  return WrapBodyInSvg(base + BodyToString());
}

} // namespace Svg
} // namespace TransportGuide
