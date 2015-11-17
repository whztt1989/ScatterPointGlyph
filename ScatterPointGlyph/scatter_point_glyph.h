#ifndef SCATTER_POINT_GLYPH_H
#define SCATTER_POINT_GLYPH_H

#include <QtWidgets/QMainWindow>
#include "ui_scatter_point_glyph.h"

#include <string>
#include <vector>

#include <vtkAutoInit.h>
#define vtkRenderingCore_AUTOINIT 3(vtkInteractionStyle,vtkRenderingFreeType,vtkRenderingOpenGL)
#define vtkRenderingVolume_AUTOINIT 1(vtkRenderingVolumeOpenGL)

class QMenu;
class QVTKWidget;
class QDockWidget;
class vtkRenderer;
class vtkUnstructuredGrid;
class vtkActor;
class LayerControlWidget;
class RenderingLayerModel;
class RenderingLayer;
class HierParaWidget;
class HierSolver;
class ClusterGlyphLayer;
class PointRenderingLayer;
class GestaltProcessor2;
class ScatterPointDataset;
class ClusterSolver;
class ScatterPointView;

class ScatterPointGlyph : public QMainWindow
{
	Q_OBJECT

public:
	ScatterPointGlyph(QWidget *parent = 0);
	~ScatterPointGlyph();

	enum SystemMode {
		HIER_MODE = 0x0,
		PERCEPTION_MODE,
	};

protected:


private:
	Ui::ScatterPointGlyphClass ui_;
	ScatterPointView* main_view_;
	vtkRenderer* main_renderer_;

	QDockWidget* layer_control_panel_;
	LayerControlWidget* layer_control_widget_;
	RenderingLayerModel* rendering_layer_model_;

	QDockWidget* hier_para_panel_;
	HierParaWidget* hier_para_widget_;

	ClusterGlyphLayer* cluster_glyph_layer_;
	PointRenderingLayer* original_point_rendering_layer_;
	PointRenderingLayer* sample_point_rendering_layer_;

	SystemMode sys_mode_;
	ClusterSolver* current_solver_;

	HierSolver* hier_solver_;
	int expected_cluster_num_;

	GestaltProcessor2* gestalt_processor_;

	vtkUnstructuredGrid* scatter_point_data_;
	ScatterPointDataset* dataset_;


	void InitWidget();
	void AddPointData2View();

	void PreProcess();
	void HierarchicalPreProcess();
	void PerceptionPreProcess();

private slots:
	void OnActionOpenTriggered();
	void OnActionCloseTriggered();
	void OnActionExitTriggered();

	void OnActionExtractDataTriggered();

	void OnMainViewUpdated();

	void OnHierClusterNumberChanged(int);
	void OnActionHierarchicalClusteringTriggered();

	void OnActionPerceptionDrivenTriggered();
	
	void OnClusterAggregated(int cluster_index);
	void OnClusterFinished();

	void NormalizePosition(std::vector< std::vector< float > >& vec);
	void NormalizeValues(std::vector< std::vector< float > >& vec);
};

#endif // SCATTER_POINT_GLYPH_H
