#ifndef VARIABLE_ITEM_H_
#define VARIABLE_ITEM_H_

#include <QtCore/QObject>
#include <QtWidgets/QGraphicsItem>
#include "tree_common.h"

#include <map>

class VariableItem : public QObject, public QGraphicsItem
{
	Q_OBJECT

public:
	VariableItem(int var_index);
	~VariableItem();

	void SetData(QString var_name, std::vector< float >& var_values, std::vector< int >& node_count, std::vector< QColor >& node_color, int selected_count);
	void SetAbsWidthEnabled(bool enabled);
	void SetHighlightEnabled(bool enabled);

signals:
	void VarSelected(int);

protected:
	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget /* = 0 */);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
	int var_index_;

	std::vector< float > var_values_;
	std::vector< int > node_count_;
	std::vector< QColor > node_color_;
	QString var_name_;
	int selected_count_;
	int total_node_count_;

	bool is_abs_width_on_;
	bool is_highlight_on_;

	int item_size = 30;
	int item_margin = 3;
	int total_width = 500;
	int total_height = 30;
};

#endif