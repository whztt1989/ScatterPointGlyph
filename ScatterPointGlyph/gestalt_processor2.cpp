#include "gestalt_processor2.h"
#include <iostream>
#include <time.h>

#include "scatter_point_dataset.h"
#include "gestalt_candidate_set.h"
#include "similarity_extractor.h"
#include "proximity_extractor.h"
#include "./gco-v3.0/GCoptimization.h"
#include "./gco-v3.0/energy.h"

GestaltProcessor2::GestaltProcessor2() 
	: ClusterSolver(), dataset_(NULL), gestalt_candidates_(NULL) {
	PropertyExtractor* proximity = new PropertyExtractor;
	SimilarityExtractor* similarity = new SimilarityExtractor;

	property_extractors_.push_back(proximity);
	property_extractors_.push_back(similarity);

	is_property_on_.resize(5, false);
	is_property_on_[0] = true;
	is_property_on_[1] = true;

	final_label_count_ = 0;
	valid_decreasing_rate_ = 0.3;
}

GestaltProcessor2::~GestaltProcessor2() {
	if (gestalt_candidates_ != NULL) delete gestalt_candidates_;
}

void GestaltProcessor2::SetData(ScatterPointDataset* data) {
	dataset_ = data;
	if (gestalt_candidates_ != NULL) delete gestalt_candidates_;
	gestalt_candidates_ = new GestaltCandidateSet(dataset_);
}

void GestaltProcessor2::GenerateCluster(float dis_thresh) {
	if (gestalt_candidates_ == NULL) return;

	final_label_.resize(gestalt_candidates_->site_num);
	final_label_.assign(final_label_.size(), -1);

	gestalt_candidates_->ExtractGestaltCandidates(dis_thresh);

	int labeled_site_count = 0;
	final_label_count_ = 0;
	while (labeled_site_count < gestalt_candidates_->site_num) {
		ExtractLabels();
		// process valid gestalt
		int property_index, gestalt_index;
		ExtractValidGestalt(property_index, gestalt_index);
		if (property_index == -1  || gestalt_index == -1) break;
		PropertyExtractor* extractor = property_extractors_[property_index];
		for (int i = 0; i < extractor->proposal_gestalt[gestalt_index].size(); ++i) {
			int site_index = extractor->proposal_gestalt[gestalt_index][i];
			final_label_[site_index] = final_label_count_;
		}

		emit ClusterUpdated(final_label_count_);

		final_label_count_++;
		labeled_site_count += extractor->proposal_gestalt[gestalt_index].size();
	}
}

void GestaltProcessor2::ExtractLabels() {
	try {
		int label_num = gestalt_candidates_->gestalt_candidates.size() + 1;
		int gestalt_num = gestalt_candidates_->gestalt_candidates.size();
		int site_num = gestalt_candidates_->site_num;

		// Step 1: construct class
		GCoptimizationGeneralGraph* gc = new GCoptimizationGeneralGraph(site_num, label_num);

		// Step 2: multi-label graph cut
		for (int i = 0; i < site_num - 1; ++i)
			for (int j = i + 1; j < site_num; ++j) {
				if (gestalt_candidates_->site_connecting_status[i][j]) gc->setNeighbors(i, j);
			}

		for (int p = 0; p < is_property_on_.size(); ++p)
			if (is_property_on_[p]) {
				PropertyExtractor* extractor = property_extractors_[p];
				extractor->SetData(dataset_, gestalt_candidates_);
				extractor->ExtractCosts(property_thresh[p]);

				for (int i = 0; i < site_num; ++i) {
					for (int j = 0; j < label_num + 1; ++j) {
						gc->setDataCost(i, j, (int)(extractor->data_cost[i][j] * 10000));
					}
				}
				for (int i = 0; i < label_num; ++i)
					for (int j = 0; j < label_num; ++j)
						gc->setSmoothCost(i, j, (int)(extractor->smooth_cost[i][j] * 10000));

				std::vector< int > temp_label_cost;
				temp_label_cost.resize(label_num);
				for (int i = 0; i < label_num; ++i) temp_label_cost[i] = (int)(extractor->label_cost[i] * 10000);
				gc->setLabelCost(temp_label_cost.data());

				std::cout << "Property: " << p << "    Before optimization energy is " << gc->compute_energy() << std::endl;
				gc->expansion(2);// run expansion for 2 iterations. For swap use gc->swap(num_iterations);
				std::cout << "Property: " << p << "    After optimization energy is " << gc->compute_energy() << std::endl;

				extractor->result_label.resize(site_num);
				for (int i = 0; i < site_num; ++i) extractor->result_label[i] = gc->whatLabel(i);
			}

		delete gc;
	}
	catch (GCException e) {
		e.Report();
	}
}

void GestaltProcessor2::ExtractValidGestalt(int& property_index, int& gestalt_index) {
	std::vector< GestaltInfo > gestalt_infos;

	for (int i = 0; i < is_property_on_.size(); ++i)
		if (is_property_on_[i]) {
			PropertyExtractor* extractor = property_extractors_[i];
			for (int j = 0; j < extractor->proposal_gestalt.size(); ++j) {
				GestaltInfo info_item;
				info_item.property_index = i;
				info_item.gestalt_index = j;

				info_item.values[0] = 0;
				for (int k = 0; k < extractor->proposal_gestalt[j].size(); ++k) {
					int site_index = extractor->proposal_gestalt[j][k];
					info_item.values[0] += extractor->data_cost[site_index][j];
				}

				info_item.values[1] = 0;
				for (int k = 0; k < extractor->proposal_gestalt[j].size(); ++k)
					if (extractor->result_label[k] == j) info_item.values[1] += 1;
				info_item.values[1] = (extractor->proposal_gestalt[j].size() - info_item.values[1]) / extractor->proposal_gestalt[j].size();
			}
		}
	
	SortGestaltInfos(gestalt_infos, 0, gestalt_infos.size() - 1, 0);
	int end_index = 1;
	while (end_index < gestalt_infos.size() && gestalt_infos[end_index].values[1] < valid_decreasing_rate_) end_index++;
	SortGestaltInfos(gestalt_infos, 0, end_index - 1, 1);

	property_index = gestalt_infos[0].property_index;
	gestalt_index = gestalt_infos[0].gestalt_index;
}

void GestaltProcessor2::SortGestaltInfos(std::vector< GestaltInfo >& infos, int begin, int end, int sort_index) {
	if (begin >= end) return;

	int first = begin, last = end;
	GestaltInfo key = infos[first];
	while (first < last) {
		while (first < last && infos[last].values[sort_index >= key.values[sort_index]]) --last;
		infos[first] = infos[last];
		while (first < last && infos[first].values[sort_index] <= key.values[sort_index]) ++first;
		infos[last] = infos[first];
	}
	infos[first] = key;
	SortGestaltInfos(infos, begin, first - 1, sort_index);
	SortGestaltInfos(infos, first + 1, end, sort_index);
}