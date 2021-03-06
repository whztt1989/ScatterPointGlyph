#include "parallel_coordinate.h"
#include <QtGui/QMouseEvent>
#include "tour_path_generator.h"

ParallelCoordinate::ParallelCoordinate()
    : dataset_(NULL),
	highlight_var_index_(-1) {
    //this->setMinimumSize(200, 300);
}

ParallelCoordinate::~ParallelCoordinate(){

}

void ParallelCoordinate::SetData(ParallelDataset* dataset_t){
    dataset_ = dataset_t;

    connect(dataset_, SIGNAL(DataChanged()), this, SLOT(OnDataChanged()));

    this->UpdateView();
}

void ParallelCoordinate::OnDataChanged() {
    this->UpdateView();
}

void ParallelCoordinate::UpdateView() {
    UpdateViewLayoutParameters();

    this->updateGL();
}

void ParallelCoordinate::SetHighlightAxis(int var_index) {
	this->highlight_var_index_ = var_index;

	this->updateGL();
}

void ParallelCoordinate::UpdateViewLayoutParameters(){
    x_border_ = 40.0 / this->width();
    y_border_ = 10.0 / this->height();

    icon_width_ = 20.0 / this->width();
    icon_height_ = 20.0 / this->height();

    if ( dataset_ == NULL ) return;

    axis_name_height_ = 15.0 / this->height();
    range_text_height_ = 10.0 / this->height();
    axis_width_ = 2.0 / this->width();

    weight_circle_radius_ = 10.0;

    subset_rect_width_ = 40.0 / this->width();
    subset_rect_height_ = 15.0 / this->height();
    subset_rect_text_width_ = 40.0 / this->width();
    subset_rect_y_value_ = 1.0 - y_border_ - subset_rect_height_;

    if ( dataset_->is_axis_weight_enabled ){
        weight_circle_radius_ = 15.0 / this->height();
        weight_circle_center_y_value_ = 0.5 * y_border_ + weight_circle_center_y_value_;
    } else {
        weight_circle_radius_ = 0;
    }

    axis_top_y_value_ = 1.0 - y_border_ * 3 - range_text_height_ - subset_rect_height_;
    axis_bottom_y_value_ = y_border_ * 2.5 + axis_name_height_ + range_text_height_ + 2 * weight_circle_radius_;
    axis_y_size_ = axis_top_y_value_ - axis_bottom_y_value_;
    axis_x_pos_values_.resize(dataset_->axis_names.size());
    int axis_num = dataset_->axis_names.size();
    float begin_pos = x_border_ + axis_width_ / 2;
    float end_pos = 1.0 - x_border_ - axis_width_ / 2;
    float step_size = (end_pos - begin_pos) / (axis_num - 1);
    for ( int i = 0; i < axis_num; ++i ) axis_x_pos_values_[i] = begin_pos + step_size * i;

    axis_name_y_value_ = y_border_ + 2 * weight_circle_radius_;
    range_text_bottom_y_value_ = axis_name_y_value_ + axis_name_height_ + 0.5 * y_border_;
    range_text_top_y_value_ = axis_top_y_value_ + 3.0 / this->height();
}

void ParallelCoordinate::initializeGL(){
    if ( glewInit() != GLEW_OK ){
        std::cout << "Error initialize PCP error!" << std::endl;
        return;
    }

    glClearColor(1.0, 1.0, 1.0, 1.0);

    QImage image("../Plots/Resources/setting.png");
    setting_texture_ = this->bindTexture(image, GL_TEXTURE_2D, GL_RGBA);
    if ( setting_texture_ == -1 ) exit(0);
}

void ParallelCoordinate::resizeGL(int w, int h){
    UpdateViewLayoutParameters();
}

void ParallelCoordinate::paintGL(){
    glViewport(0, 0, this->width(), this->height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, 0, 2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -1.0);

    glClear(GL_COLOR_BUFFER_BIT);
    glShadeModel(GL_SMOOTH);

    if ( dataset_ == NULL || dataset_->is_updating) return;

    PaintSubsetIdentifyItems();
    if (!dataset_->is_gaussian_enabled) PaintLines();
	else PaintGaussianCurve();
    PaintText();
    PaintCoordinate();
}

void ParallelCoordinate::PaintSettingIcon(){
    this->drawTexture(QRectF(1.0 - icon_width_, 1.0 - icon_height_, icon_width_, icon_height_), setting_texture_);
}

void ParallelCoordinate::PaintCoordinate(){
    for ( int i = 0; i < axis_x_pos_values_.size(); ++i ){
		glColor3f(0.0, 0.0, 0.0);
        glRectf(axis_x_pos_values_[i] - axis_width_ / 2, axis_bottom_y_value_, axis_x_pos_values_[i] + axis_width_ / 2, axis_top_y_value_);

		if (i == highlight_var_index_) {
			float centerx = axis_x_pos_values_[i];
			float centery = axis_top_y_value_;

			glColor3f(1.0, 0.0, 0.0);
			glBegin(GL_POLYGON);
			for (int j = 0; j <= 30; ++j) {
				float end_arc = j * 3.14159 * 2 / 30;
				float x = centerx + icon_width_ * 0.3 * cos(end_arc);
				float y = centery + icon_height_ * 0.3 * sin(end_arc);

				glVertex3f(x, y, 0);
			}
			glEnd();

			centery = axis_bottom_y_value_;

			glColor3f(1.0, 0.0, 0.0);
			glBegin(GL_POLYGON);
			for (int j = 0; j <= 30; ++j) {
				float end_arc = j * 3.14159 * 2 / 30;
				float x = centerx + icon_width_ * 0.3 * cos(end_arc);
				float y = centery + icon_height_ * 0.3 * sin(end_arc);

				glVertex3f(x, y, 0);
			}
			glEnd();
		}
    }
}

void ParallelCoordinate::PaintLines(){
	glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for ( int i = 0; i < dataset_->subset_records.size(); ++i ){
        if ( i != 0 ) 
            glLineWidth(2.0);
        else
            glLineWidth(1.0);
        //glColor4f(dataset_->subset_colors[i].redF(), dataset_->subset_colors[i].greenF(), dataset_->subset_colors[i].blueF(), dataset_->subset_opacity[i]);
        for ( int j = 0; j < dataset_->subset_records[i].size(); ++j ){
            ParallelRecord* record = dataset_->subset_records[i][j];
            if ( record == NULL || record->values.size() != dataset_->axis_names.size() ) continue;
            if ( dataset_->is_record_selected.size() != 0 && dataset_->record_color.size() != 0 ){
                if ( dataset_->is_record_selected[i][j] ){
                    glColor4f(dataset_->record_color[i][j].redF(), dataset_->record_color[i][j].greenF(), dataset_->record_color[i][j].blueF(), 0.5);
                } else {
                    glColor4f(dataset_->record_color[i][j].redF(), dataset_->record_color[i][j].greenF(), dataset_->record_color[i][j].blueF(), 0.0);
                }
            }
            glBegin(GL_LINE_STRIP);
            for ( int k = 0; k < record->values.size(); ++k )
				glVertex3f(axis_x_pos_values_[k], axis_bottom_y_value_ + record->values[dataset_->mapped_axis[k]] * axis_y_size_, 0);
            glEnd();
        }
    }

    glDisable(GL_BLEND);
}

void ParallelCoordinate::PaintGaussianCurve() {
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (int i = 0; i < dataset_->subset_records.size(); ++i){
		if (i != 0)
			glLineWidth(2.0);
		else
			glLineWidth(1.0);

		for (int j = 0; j < dataset_->axis_names.size() - 1; ++j){
			float bottom_y = dataset_->var_centers[i][dataset_->mapped_axis[j]] - dataset_->var_width[i][dataset_->mapped_axis[j]];
			if (bottom_y < 0) bottom_y = 0;
			float top_y = dataset_->var_centers[i][dataset_->mapped_axis[j]] + dataset_->var_width[i][dataset_->mapped_axis[j]];
			if (top_y > 1) top_y = 1;

			float next_bottom_y = dataset_->var_centers[i][dataset_->mapped_axis[j + 1]] - dataset_->var_width[i][dataset_->mapped_axis[j + 1]];
			if (next_bottom_y < 0) next_bottom_y = 0;
			float next_top_y = dataset_->var_centers[i][dataset_->mapped_axis[j + 1]] + dataset_->var_width[i][dataset_->mapped_axis[j + 1]];
			if (next_top_y > 1) next_top_y = 1;

			glBegin(GL_TRIANGLE_STRIP);
				glColor4f(dataset_->subset_colors[i].redF(), dataset_->subset_colors[i].greenF(), dataset_->subset_colors[i].blueF(), 0.7);
				glVertex3f(axis_x_pos_values_[j], axis_bottom_y_value_ + dataset_->var_centers[i][dataset_->mapped_axis[j]] * axis_y_size_, 0);
				glColor4f(dataset_->subset_colors[i].redF(), dataset_->subset_colors[i].greenF(), dataset_->subset_colors[i].blueF(), 0.5);
				glVertex3f(axis_x_pos_values_[j], axis_bottom_y_value_ + bottom_y * axis_y_size_, 0);
				glColor4f(dataset_->subset_colors[i].redF(), dataset_->subset_colors[i].greenF(), dataset_->subset_colors[i].blueF(), 0.7);
				glVertex3f(axis_x_pos_values_[j + 1], axis_bottom_y_value_ + dataset_->var_centers[i][dataset_->mapped_axis[j + 1]] * axis_y_size_, 0);
				glColor4f(dataset_->subset_colors[i].redF(), dataset_->subset_colors[i].greenF(), dataset_->subset_colors[i].blueF(), 0.5);
				glVertex3f(axis_x_pos_values_[j + 1], axis_bottom_y_value_ + next_bottom_y * axis_y_size_, 0);
			glEnd();
			glBegin(GL_TRIANGLE_STRIP);
				glColor4f(dataset_->subset_colors[i].redF(), dataset_->subset_colors[i].greenF(), dataset_->subset_colors[i].blueF(), 0.7);
				glVertex3f(axis_x_pos_values_[j], axis_bottom_y_value_ + dataset_->var_centers[i][dataset_->mapped_axis[j]] * axis_y_size_, 0);
				glColor4f(dataset_->subset_colors[i].redF(), dataset_->subset_colors[i].greenF(), dataset_->subset_colors[i].blueF(), 0.5);
				glVertex3f(axis_x_pos_values_[j], axis_bottom_y_value_ + top_y * axis_y_size_, 0);
				glColor4f(dataset_->subset_colors[i].redF(), dataset_->subset_colors[i].greenF(), dataset_->subset_colors[i].blueF(), 0.7);
				glVertex3f(axis_x_pos_values_[j + 1], axis_bottom_y_value_ + dataset_->var_centers[i][dataset_->mapped_axis[j + 1]] * axis_y_size_, 0);
				glColor4f(dataset_->subset_colors[i].redF(), dataset_->subset_colors[i].greenF(), dataset_->subset_colors[i].blueF(), 0.5);
				glVertex3f(axis_x_pos_values_[j + 1], axis_bottom_y_value_ + next_top_y * axis_y_size_, 0);
			glEnd();
		}
	}

	glDisable(GL_BLEND);
}

void ParallelCoordinate::PaintText(){
    int y_axis_name = (int)((1.0 - axis_name_y_value_) * this->height());
    int y_max = (int)((1.0 - range_text_top_y_value_) * this->height()) - 10;
    int y_min = (int)((1.0 - range_text_bottom_y_value_) * this->height());

    glColor3f(0.0, 0.0, 0.0);
    for ( int i = 0; i < axis_x_pos_values_.size(); ++i ){
        int axis_x = (int)(axis_x_pos_values_[i] * this->width());
		this->renderText(axis_x - dataset_->axis_names[dataset_->mapped_axis[i]].length() * 3, y_axis_name, dataset_->axis_names[dataset_->mapped_axis[i]]);
		this->renderText(axis_x - dataset_->axis_anchors[dataset_->mapped_axis[i]][1].length() * 3, y_max, dataset_->axis_anchors[dataset_->mapped_axis[i]][1]);
		this->renderText(axis_x - dataset_->axis_anchors[dataset_->mapped_axis[i]][0].length() * 3, y_min, dataset_->axis_anchors[dataset_->mapped_axis[i]][0]);
    }
}

void ParallelCoordinate::PaintSubsetIdentifyItems(){
    for ( int i = 0; i < dataset_->subset_names.size(); ++i ){
        float x = i * (subset_rect_width_ + subset_rect_text_width_ + x_border_) + x_border_;
        glColor3f(dataset_->subset_colors[i].redF(), dataset_->subset_colors[i].greenF(), dataset_->subset_colors[i].blueF());
        glRectf(x, subset_rect_y_value_, x + subset_rect_width_, subset_rect_y_value_ + subset_rect_height_);

        int text_x = (x + subset_rect_width_) * this->width() + 5;
        int text_y = (1.0 - subset_rect_y_value_) * this->height();
        glColor3f(0.0, 0.0, 0.0);
        this->renderText(text_x, text_y, dataset_->subset_names[i]);
    }
}

void ParallelCoordinate::PaintWeightCircles(){

}

void ParallelCoordinate::mouseDoubleClickEvent(QMouseEvent *event) {
	if (dataset_ != NULL) {
		dataset_->is_gaussian_enabled = !dataset_->is_gaussian_enabled;
		this->update();
	}
}

void ParallelCoordinate::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::RightButton) {
		int index = -1;
		float min_dis = 1e10;
		float x = (float)event->x() / this->width();
		for (int i = 0; i < axis_x_pos_values_.size(); ++i)
			if (abs(axis_x_pos_values_[i] - x) < min_dis) {
				min_dis = abs(axis_x_pos_values_[i] - x);
				index = i;
			}
		if (index == highlight_var_index_) {
			this->highlight_var_index_ = -1;
		} else {
			this->highlight_var_index_ = index;
		}
		emit HighlightVarChanged(this->highlight_var_index_);

		this->updateGL();
	}
}