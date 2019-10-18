#include <objective_functions/Lagrangian.h>

Lagrangian::Lagrangian()
{
	name = "Lagrangian";
	w = 0;
}

void Lagrangian::init()
{
	if (V.size() == 0 || F.size() == 0)
		throw name + " must define members V,F before init()!";
	
	a.resize(F.rows());
	b.resize(F.rows());
	c.resize(F.rows());
	d.resize(F.rows());
	
	//Parameterization J mats resize
	detJ.resize(F.rows());
	grad.resize(F.rows(), 6);
	Hessian.resize(F.rows());
	dJ_dX.resize(F.rows());
	lambda.resize(F.rows());
	
	// compute init energy matrices
	igl::doublearea(V, F, Area);
	Area /= 2;

	MatrixX3d D1cols, D2cols;

	Utils::computeSurfaceGradientPerFace(V, F, D1cols, D2cols);
	D1d = D1cols.transpose();
	D2d = D2cols.transpose();

	//prepare dJ/dX
	for (int i = 0; i < F.rows(); i++) {
		MatrixXd Dx = D1d.col(i).transpose();
		MatrixXd Dy = D2d.col(i).transpose();
		MatrixXd zero = VectorXd::Zero(3).transpose();
		dJ_dX[i] << 
			Dx	, zero	,
			zero, Dx	,
			Dy	, zero	,
			zero, Dy;
	}

	init_hessian();
}

void Lagrangian::updateX(const VectorXd& X)
{
	bool inversions_exist = update_variables(X);
	if (inversions_exist) {
		cout << name << " Error! inversion exists." << endl;
	}
}

double Lagrangian::value(bool update)
{
	// L = LSCM - lambda * area
	VectorXd LSCM = 2 * (d.cwiseAbs2()) + (b + c).cwiseAbs2() + 2 * (a.cwiseAbs2());
	VectorXd areaE = detJ - VectorXd::Ones(F.rows());
	
	VectorXd E = LSCM - lambda.cwiseProduct(areaE);
	double value = (Area.asDiagonal() * E).sum();
	
	if (update) {
		Efi = E;
		energy_value = value;
	}
	
	return value;
}

double Lagrangian::AugmentedValue()
{
	// L = LSCM - lambda * area
	double k = 1;

	VectorXd areaE = (detJ - VectorXd::Ones(F.rows())).cwiseAbs2();
	//I am not sure of multiplying areaE by Area!!!
	double augmented_part = (Area.asDiagonal() * areaE).sum();
	
	return value(false) + k* augmented_part;
}

void Lagrangian::gradient(VectorXd& g)
{
	g.conservativeResize(V.rows() * 2 + F.rows());
	g.setZero();

	for (int fi = 0; fi < F.rows(); ++fi) {
		//prepare gradient
		Vector4d dE_dJ(
			4 * a(fi) - lambda(fi) * d(fi), 
			2 * b(fi) + 2 * c(fi) + lambda(fi) * c(fi),
			2 * b(fi) + 2 * c(fi) + lambda(fi) * b(fi),
			4 * d(fi) - lambda(fi) * a(fi)
		);
		grad.row(fi) = Area(fi)*(dE_dJ.transpose() * dJ_dX[fi]).transpose();
		
		
		//Update the gradient of the x-axis
		g(F(fi, 0)) += grad(fi, 0);
		g(F(fi, 1)) += grad(fi, 1);
		g(F(fi, 2)) += grad(fi, 2);
		//Update the gradient of the y-axis
		g(F(fi, 0) + V.rows()) += grad(fi, 3);
		g(F(fi, 1) + V.rows()) += grad(fi, 4);
		g(F(fi, 2) + V.rows()) += grad(fi, 5);
		//Update the gradient of lambda
		g(fi + 2 * V.rows()) += detJ(fi) - 1;
	}
	gradient_norm = g.norm();
}

void Lagrangian::hessian()
{
#pragma omp parallel for num_threads(24)
	int index2 = 0;
	for (int i = 0; i < F.rows(); ++i) {
		double detj_1 = (a(i) * d(i) - b(i) * c(i)) - 1;

		//prepare hessian
		MatrixXd d2E_dJ2(4, 4);
		d2E_dJ2 <<
			d(i)*d(i)			, -c(i)*d(i)			, -b(i)*d(i)		, a(i)*d(i) + detj_1,
			-c(i)*d(i)			, c(i)*c(i)				, b(i)*c(i) - detj_1, -c(i)*a(i),
			-b(i)*d(i)			, b(i)*c(i) - detj_1	, b(i)*b(i)			, -b(i)*a(i),
			a(i)*d(i) + detj_1	, -a(i)*c(i)			, -a(i)*b(i)		, a(i)*a(i);

		Hessian[i] = Area(i) * dJ_dX[i].transpose() * d2E_dJ2 * dJ_dX[i];

		for (int a = 0; a < 6; ++a)
		{
			for (int b = 0; b <= a; ++b)
			{
				SS[index2++] = Hessian[i](a, b);
			}
		}
	}
}

bool Lagrangian::update_variables(const VectorXd& X)
{
	lambda = X.tail(F.rows());
	Eigen::Map<const MatrixX2d> x(X.head(2*V.rows()).data(), X.head(2 * V.rows()).size() / 2, 2);

	for (int i = 0; i < F.rows(); i++)
	{
		Vector3d Xi, Yi;
		Xi << x(F(i, 0), 0), x(F(i, 1), 0), x(F(i, 2), 0);
		Yi << x(F(i, 0), 1), x(F(i, 1), 1), x(F(i, 2), 1);
		Vector3d Dx = D1d.col(i);
		Vector3d Dy = D2d.col(i);

		//prepare jacobian		
		a(i) = Dx.transpose() * Xi;
		b(i) = Dx.transpose() * Yi;
		c(i) = Dy.transpose() * Xi;
		d(i) = Dy.transpose() * Yi;
	}
	detJ = a.cwiseProduct(d) - b.cwiseProduct(c);

	return ((detJ.array() < 0).any());
}

void Lagrangian::init_hessian()
{
	II.clear();
	JJ.clear();
	auto PushPair = [&](int i, int j) { if (i > j) swap(i, j); II.push_back(i); JJ.push_back(j); };
	int n = V.rows();
	for (int i = 0; i < F.rows(); ++i)
		AddElementToHessian({ F(i, 0), F(i, 1), F(i, 2), F(i, 0) + n, F(i, 1) + n, F(i, 2) + n });
	SS = vector<double>(II.size(), 0.);
}