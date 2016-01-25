#include "hierarchical_tree.h"
#include <queue>
#include "scatter_point_dataset.h"

HierarchicalTree::HierarchicalTree(ScatterPointDataset* data) 
	: TreeCommon(data), expected_cluster_num_(-1), type_(CENTER_DISTANCE),
	data_dis_scale_(0.5) {
}

HierarchicalTree::~HierarchicalTree() {

}

void HierarchicalTree::SetExpectedClusterNum(int num) {
	this->expected_cluster_num_ = num;
}

void HierarchicalTree::SetDistanceType(DistanceType type) {
	this->type_ = type;
}

void HierarchicalTree::run() {
	if (root_->linked_nodes.size() != 0) {
		for (int i = 0; i < root_->linked_nodes.size(); ++i) delete root_->linked_nodes[i];
		root_->linked_nodes.clear();
	}

	this->ConstructDirectly();

	if (expected_cluster_num_ < 0) expected_cluster_num_ = 10;

	this->GenerateCluster();

	ResetLevel(root_, 0);

	AssignColor(root_, 0, 1.0);

	this->InitializeSortingIndex();
}

void HierarchicalTree::GenerateCluster() {
	while (root_->linked_nodes.size() > expected_cluster_num_) {
		int min_node_index[2];
		float min_node_dis = 1e20;
		min_node_index[0] = -1;
		min_node_index[1] = -1;

		for (int i = 0; i < root_->linked_nodes.size() - 1; ++i)
			for (int j = i + 1; j < root_->linked_nodes.size(); ++j) {
				float temp_dis = 0;

				float value_dis = 0;
				for (int k = 0; k < dataset_->var_num; ++k)
					value_dis += abs(root_->linked_nodes[i]->average_values[k] - root_->linked_nodes[j]->average_values[k]) * dataset_->var_weights[k];

				float pos_dis = sqrt(pow(root_->linked_nodes[i]->center_pos[0] - root_->linked_nodes[j]->center_pos[0], 2) + pow(root_->linked_nodes[i]->center_pos[1] - root_->linked_nodes[j]->center_pos[1], 2));

				temp_dis = data_dis_scale_ * value_dis + (1.0 - data_dis_scale_) * pos_dis;

				if (temp_dis < min_node_dis) {
					min_node_index[0] = i;
					min_node_index[1] = j;
					min_node_dis = temp_dis;
				}
			}

		if (min_node_index[0] != -1 && min_node_index[1] != -1) {
			CBranch* branch = new CBranch;
			branch->linked_nodes.push_back(root_->linked_nodes[min_node_index[0]]);
			branch->linked_nodes.push_back(root_->linked_nodes[min_node_index[1]]);
			ProgressNode(branch);

			root_->linked_nodes[min_node_index[0]] = branch;
			for (int i = min_node_index[1]; i < root_->linked_nodes.size() - 1; ++i)
				root_->linked_nodes[i] = root_->linked_nodes[i + 1];
			root_->linked_nodes.resize(root_->linked_nodes.size() - 1);
		}
	}
}