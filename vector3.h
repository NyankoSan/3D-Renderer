#pragma once

class Vector3 {
public:
	double x_, y_, z_;
	Vector3();
	Vector3(double, double, double);
	
	Vector3 operator+(Vector3);
	Vector3 operator-(Vector3);
	Vector3 operator*(double);
	Vector3 operator/(double);
	void operator+=(Vector3);

	void normalize();

	double length();
	double lengthsqr();
};

double dot_prod(Vector3, Vector3);
Vector3 elem_mult(Vector3, Vector3);
Vector3 cross_prod(Vector3, Vector3);