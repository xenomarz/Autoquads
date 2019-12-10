#include <objective_functions/AreaDistortionOneRing.h>
#include <igl/vertex_triangle_adjacency.h>
AreaDistortionOneRing::AreaDistortionOneRing()
{
	name = "One Ring Area Preserving";
}

void AreaDistortionOneRing::init()
{
	if (V.size() == 0 || F.size() == 0)
		throw name + " must define members V,F before init()!";
	
	// compute init energy matrices
	igl::doublearea(V, F, Area);
	Area /= 2;
	igl::vertex_triangle_adjacency(V, F, VF, VFi);

	a.resize(F.rows());
	b.resize(F.rows());
	c.resize(F.rows());
	d.resize(F.rows());
	
	//Parameterization J mats resize
	detJ.resize(F.rows());
	OneRingSum.resize(V.rows());
	grad.resize(V.rows());
	Hessian.resize(V.rows());
	dJ_dX.resize(V.rows());
	OneRingVertices.resize(V.rows());
	dE_dJ.resize(V.rows());

	for (int vi = 0; vi < V.rows(); vi++) {
		vector<int> OneRingFaces = VF[vi];
		OneRingVertices[vi] = get_one_ring_vertices(OneRingFaces);
	}

	MatrixX3d D1cols, D2cols;
	Utils::computeSurfaceGradientPerFace(V, F, D1cols, D2cols);
	D1d = D1cols.transpose();
	D2d = D2cols.transpose();

	init_dJdX();
	init_hessian();
}

vector<int> AreaDistortionOneRing::get_one_ring_vertices(const vector<int>& OneRingFaces) {
	vector<int> vertices;
	vertices.clear();
	for (int i = 0; i < OneRingFaces.size(); i++) {
		int fi = OneRingFaces[i];
		int P0 = F(fi, 0);
		int P1 = F(fi, 1);
		int P2 = F(fi, 2);

		//check if the vertex already exist
		if (!(find(vertices.begin(), vertices.end(), P0) != vertices.end())) {
			vertices.push_back(P0);
		}
		if (!(find(vertices.begin(), vertices.end(), P1) != vertices.end())) {
			vertices.push_back(P1);
		}
		if (!(find(vertices.begin(), vertices.end(), P2) != vertices.end())) {
			vertices.push_back(P2);
		}
	}
	return vertices;
}

void AreaDistortionOneRing::updateX(const VectorXd& X)
{
	bool inversions_exist = update_variables(X);
	if (inversions_exist) {
		//cout << name << " Error! inversion exists." << endl;
	}
}

double AreaDistortionOneRing::value(const bool update)
{
	double value = OneRingSum.cwiseAbs2().sum();
	value /= 2;

	if (update) {
		Efi.setZero();
		energy_value = value;
	}
	
	return value;
}

void AreaDistortionOneRing::gradient(VectorXd& g)
{
	g.conservativeResize(V.rows() * 2);
	g.setZero();

	for (int vi = 0; vi < V.rows(); ++vi) {
		vector<int> OneRingFaces = VF[vi];
		int J_size = 4 * OneRingFaces.size();

		dE_dJ[vi].resize(1, J_size);
		dE_dJ[vi].setZero();

		//prepare gradient
		for (int i = 0; i < OneRingFaces.size(); i++) {
			int fi = OneRingFaces[i];
			int base_column = 4 * i;
			dE_dJ[vi].block<1, 4>(0, base_column) = Area(fi)*Vector4d(d(fi), -c(fi), -b(fi), a(fi));
		}
		grad[vi] = OneRingSum(vi)*dE_dJ[vi] * dJ_dX[vi];

		for (int xi = 0; xi < OneRingVertices[vi].size(); xi++) {
			int global_xi = OneRingVertices[vi][xi];
			g(global_xi) += grad[vi](xi);
			g(global_xi + V.rows()) += grad[vi](xi + OneRingVertices[vi].size());
		}
	}
	gradient_norm = g.norm();
}

void AreaDistortionOneRing::hessian()
{
#pragma omp parallel for num_threads(24)
	int index2 = 0;
	for (int vi = 0; vi < V.rows(); ++vi) {
		vector<int> OneRingFaces = VF[vi];
		int J_size = 4 * OneRingFaces.size();
		int X_size = 2 * OneRingVertices[vi].size();

		dE_dJ[vi].resize(1, J_size);
		dE_dJ[vi].setZero();

		//prepare dE_dJ
		for (int i = 0; i < OneRingFaces.size(); i++) {
			int fi = OneRingFaces[i];
			int base_column = 4 * i;
			dE_dJ[vi].block<1, 4>(0, base_column) = Area(fi)*Vector4d(d(fi), -c(fi), -b(fi), a(fi));
		}
		MatrixXd d2E_dJ2(J_size, J_size);
		d2E_dJ2.setZero();

		//prepare hessian
		for (int i = 0; i < OneRingFaces.size(); i++) {
			int fi = OneRingFaces[i];
			int base_row = 4 * i;

			d2E_dJ2.block(base_row + 0, 0, 1, J_size) = dE_dJ[vi] * Area(fi)*d(fi);
			d2E_dJ2(base_row + 0, base_row + 3) += OneRingSum(vi)*Area(fi);

			d2E_dJ2.block(base_row + 1, 0, 1, J_size) = -1 * dE_dJ[vi] * Area(fi)*c(fi);
			d2E_dJ2(base_row + 1, base_row + 2) -= OneRingSum(vi)*Area(fi);

			d2E_dJ2.block(base_row + 2, 0, 1, J_size) = -1 * dE_dJ[vi] * Area(fi)*b(fi);
			d2E_dJ2(base_row + 2, base_row + 1) -= OneRingSum(vi)*Area(fi);

			d2E_dJ2.block(base_row + 3, 0, 1, J_size) = dE_dJ[vi] * Area(fi)*a(fi);
			d2E_dJ2(base_row + 3, base_row + 0) += OneRingSum(vi)*Area(fi);
		}
		Hessian[vi] = dJ_dX[vi].transpose() * d2E_dJ2 * dJ_dX[vi];
		
		//update the global matrix
		for (int a = 0; a < X_size; ++a)
		{
			for (int b = 0; b <= a; ++b)
			{
				SS[index2++] = Hessian[vi](a, b);
			}
		}
	}
}

bool AreaDistortionOneRing::update_variables(const VectorXd& X)
{
	Eigen::Map<const MatrixX2d> x(X.data(), X.size() / 2, 2);
	
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

	OneRingSum.setZero();
	for (int vi = 0; vi < VF.size(); vi++) {
		vector<int> OneRing = VF[vi];
		for (int fi : OneRing) {
			OneRingSum(vi) += Area(fi)*detJ(fi) - Area(fi);
		}
	}
	return ((detJ.array() < 0).any());
}

void AreaDistortionOneRing::init_hessian()
{
	II.clear();
	JJ.clear();
	auto PushPair = [&](int i, int j) { if (i > j) swap(i, j); II.push_back(i); JJ.push_back(j); };
	for (int vi = 0; vi < V.rows(); ++vi) {
		int X_size = 2 * OneRingVertices[vi].size();
		for (int a = 0; a < X_size; ++a)
		{
			for (int b = 0; b <= a; ++b)
			{
				int global_a, global_b;

				if (a >= OneRingVertices[vi].size()) {
					global_a = OneRingVertices[vi][a - OneRingVertices[vi].size()] + V.rows();
				}
				else {
					global_a = OneRingVertices[vi][a];
				}
				if (b >= OneRingVertices[vi].size()) {
					global_b = OneRingVertices[vi][b - OneRingVertices[vi].size()] + V.rows();
				}
				else {
					global_b = OneRingVertices[vi][b];
				}
				PushPair(global_a, global_b);
			}
		}
	}
	SS = vector<double>(II.size(), 0.);
}

void AreaDistortionOneRing::init_dJdX() {
	//prepare dJ/dX
	for (int vi = 0; vi < VF.size(); vi++) {
		vector<int> OneRingFaces = VF[vi];

		int J_size = 4 * OneRingFaces.size();
		int X_size = 2 * OneRingVertices[vi].size();

		dJ_dX[vi].resize(J_size, X_size);
		dJ_dX[vi].setZero();
		for (int i = 0; i < OneRingFaces.size(); i++) {
			int fi = OneRingFaces[i];
			int base_row = 4 * i;
			RowVectorXd Dx = D1d.col(fi).transpose();
			RowVectorXd Dy = D2d.col(fi).transpose();

			//Find the indexes of the face's vertices (p0,p1,p2) on the gradient vector
			int x0 = distance(OneRingVertices[vi].begin(), find(OneRingVertices[vi].begin(), OneRingVertices[vi].end(), F(fi, 0)));
			int x1 = distance(OneRingVertices[vi].begin(), find(OneRingVertices[vi].begin(), OneRingVertices[vi].end(), F(fi, 1)));
			int x2 = distance(OneRingVertices[vi].begin(), find(OneRingVertices[vi].begin(), OneRingVertices[vi].end(), F(fi, 2)));
			int y0 = x0 + OneRingVertices[vi].size();
			int y1 = x1 + OneRingVertices[vi].size();
			int y2 = x2 + OneRingVertices[vi].size();

			// update the gradient for: 
			// X cordinate of the first vertex 
			dJ_dX[vi](base_row + 0, x0) += Dx(0);
			dJ_dX[vi](base_row + 2, x0) += Dy(0);
			// X cordinate of the second vertex
			dJ_dX[vi](base_row + 0, x1) += Dx(1);
			dJ_dX[vi](base_row + 2, x1) += Dy(1);
			// X cordinate of the third vertex
			dJ_dX[vi](base_row + 0, x2) += Dx(2);
			dJ_dX[vi](base_row + 2, x2) += Dy(2);

			// Y cordinate of the first vertex
			dJ_dX[vi](base_row + 1, y0) += Dx(0);
			dJ_dX[vi](base_row + 3, y0) += Dy(0);
			// Y cordinate of the second vertex
			dJ_dX[vi](base_row + 1, y1) += Dx(1);
			dJ_dX[vi](base_row + 3, y1) += Dy(1);
			// Y cordinate of the third vertex
			dJ_dX[vi](base_row + 1, y2) += Dx(2);
			dJ_dX[vi](base_row + 3, y2) += Dy(2);
		}
	}
}