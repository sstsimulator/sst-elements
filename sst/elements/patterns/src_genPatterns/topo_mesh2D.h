/*
** $Id: topo_mesh2D.h,v 1.3 2010/04/28 21:19:51 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#ifndef _MESH2D_H_
#define _MESH2D_H_

void GenMesh2D(int net_x_dim, int net_y_dim, int NoC_x_dim, int NoC_y_dim, int num_cores,
    int IO_nodes);

#endif /* _MESH2D_H_ */
