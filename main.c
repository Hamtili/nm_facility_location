#include <stdio.h>            /* C input/output                       */
#include <stdlib.h>           /* C standard library                   */
#include <glpk.h>             /* GNU GLPK linear/mixed integer solver */
#include <time.h>
#include <math.h>
#include "gnuplot_i.h"

#define NRECEIVER 1000
#define NSTATIONS 700
#define TRADIUS 15

#define DMIN 0
#define DMAX 100

struct Position {
	double x;
	double y;
};

double drand() {
	double r = (double) rand() / RAND_MAX;
	return (DMIN + r * (DMAX - DMIN));
}

int main(void) {
	struct Position distributionFrame, receivers[NRECEIVER],
			stations[NSTATIONS];

	int i = 0, j = 0;

	glp_prob *lp;
	gnuplot_ctrl *h;

	static int ia[1 + 1000000], ja[1 + 1000000];
	static double ar[1 + 1000000];
	double x[NSTATIONS];
	double stationCost[NSTATIONS];
	int numberUsedStations = 0;
	double *xPos, *yPos, *xStart, *yStart;
	double xPosR[NRECEIVER], yPosR[NRECEIVER];

	lp = glp_create_prob();
	glp_set_prob_name(lp, "facility_location");
	glp_set_obj_dir(lp, GLP_MIN);

	glp_add_rows(lp, NRECEIVER);
	glp_add_cols(lp, NSTATIONS);

	srand(time(NULL));
	distributionFrame.x = drand();
	distributionFrame.y = drand();

	printf("Distribution Frame position: (%.0f,%.0f)\n", distributionFrame.x,
			distributionFrame.y);
	for (i = 0; i < NRECEIVER; i++) {
		glp_set_row_bnds(lp, i + 1, GLP_LO, 1.0, 1.0);
		receivers[i].x = drand();
		receivers[i].y = drand();
		xPosR[i] = receivers[i].x;
		yPosR[i] = receivers[i].y;
		printf("Receiver %i position: (%.0f,%.0f)\n", i + 1, receivers[i].x,
				receivers[i].y);

		if (i < NSTATIONS) {
			stations[i].x = drand();
			stations[i].y = drand();
			stationCost[i] = sqrt(
					pow(distributionFrame.x - stations[i].x, 2)
							+ pow(distributionFrame.y - stations[i].y, 2));
			glp_set_col_kind(lp, i + 1, GLP_BV);
			glp_set_obj_coef(lp, i + 1, stationCost[i]);
			printf("Station %i position: (%.0f,%.0f)\n", i + 1, stations[i].x,
					stations[i].y);
		}
	}

	for (i = 1; i <= NRECEIVER; i++) {
		for (j = 1; j <= NSTATIONS; j++) {
			ia[j + NSTATIONS * (i - 1)] = i;
			ja[j + NSTATIONS * (i - 1)] = j;
			if (sqrt(
					pow(receivers[i - 1].x - stations[j - 1].x, 2)
							+ pow(receivers[i - 1].y - stations[j - 1].y,
									2)) <= TRADIUS) {
				ar[j + NSTATIONS * (i - 1)] = 1;
			} else {
				ar[j + NSTATIONS * (i - 1)] = 0;
			}

		}

	}

	glp_load_matrix(lp, NRECEIVER * NSTATIONS, ia, ja, ar);
	glp_simplex(lp, NULL);
	glp_intopt(lp, NULL);

	for (i = 0; i < NSTATIONS; i++) {
		x[i] = glp_mip_col_val(lp, i + 1);
		if (x[i] == 1) {
			printf("Station %d is used. \n", i);
			numberUsedStations++;
		}
	}

	xPos = (double *) malloc(sizeof(double) * numberUsedStations);
	yPos = (double *) malloc(sizeof(double) * numberUsedStations);
	xStart = xPos;
	yStart = yPos;

	for (i = 0; i < NSTATIONS; i++) {
		if (x[i] == 1) {
			*xPos = stations[i].x;
			*yPos = stations[i].y;
			xPos++;
			yPos++;

		}
	}

	h = gnuplot_init();

	gnuplot_set_xlabel(h, "X-Position");
	gnuplot_set_ylabel(h, "Y-Position");
	gnuplot_set_ylabel(h, "Y-Position");
	gnuplot_cmd(h, "set terminal wxt");
	gnuplot_cmd(h, "set size square");

	gnuplot_cmd(h, "set parametric");
	gnuplot_cmd(h, "set xrange [0:100]");
	gnuplot_cmd(h, "set yrange [0:100]");
	gnuplot_cmd(h, "set trange [0:2*pi]");

	gnuplot_setstyle(h, "points");
	gnuplot_plot_xy(h, &distributionFrame.x, &distributionFrame.y, 1,
			"Distribution Frame");
	gnuplot_plot_xy(h, xStart, yStart, numberUsedStations, "Used Station");
	gnuplot_plot_xy(h, xPosR, yPosR, NRECEIVER, "Receiver");

	xPos = xStart;
	yPos = yStart;

	for (i = 0; i < numberUsedStations; i++) {
		gnuplot_cmd(h, "x%d(t) = %d*cos(t)+%f", i, TRADIUS, *xPos);
		gnuplot_cmd(h, "y%d(t) = %d*sin(t)+%f", i, TRADIUS, *yPos);
		gnuplot_cmd(h, "replot x%d(t), y%d(t) notitle lt rgb \"green\"", i, i);
		xPos++;
		yPos++;

	}

	free(xStart);
	free(yStart);
	getchar();
	gnuplot_close(h);
	glp_delete_prob(lp);

	return (0);
}

