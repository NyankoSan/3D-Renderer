#include <math.h>

#include "vector3.h"

Vector3::Vector3() : x_(0.0), y_(0.0), z_(0.0) {}
Vector3::Vector3(double x, double y, double z) : x_(x), y_(y), z_(z) {}

Vector3 Vector3::operator-(Vector3 o) {
	return Vector3(x_ - o.x_, y_ - o.y_, z_ - o.z_);
}
Vector3 Vector3::operator+(Vector3 o) {
	return Vector3(x_ + o.x_, y_ + o.y_, z_ + o.z_);
}
Vector3 Vector3::operator*(double v) {
	return Vector3(v * x_, v * y_, v * z_);
}
Vector3 Vector3::operator/(double v) {
	return Vector3(x_ / v, y_ / v, z_ / v);
}
void Vector3::operator+=(Vector3 o) {
	x_ += o.x_;
	y_ += o.y_;
	z_ += o.z_;
}

double Vector3::length() {
	return sqrt(x_ * x_ + y_ * y_ + z_ * z_);
}
double Vector3::lengthsqr() {
	return x_ * x_ + y_ * y_ + z_ * z_;
}
void Vector3::normalize() {
	double l = length();
	if (l == 0) return;

	x_ /= l;
	y_ /= l;
	z_ /= l;
}

double dot_prod(Vector3 v, Vector3 u) {
	return v.x_ * u.x_ + v.y_ * u.y_ + v.z_ * u.z_;
}
Vector3 elem_mult(Vector3 v, Vector3 u) {
	return Vector3(v.x_ * u.x_, v.y_ * u.y_, v.z_ * u.z_);
}
Vector3 cross_prod(Vector3 v, Vector3 u) {
	return Vector3(
		v.y_ * u.z_ - v.z_ * u.y_,
		v.z_ * u.x_ - v.x_ * u.z_,
		v.x_ * u.y_ - v.y_ * u.x_
	);
}