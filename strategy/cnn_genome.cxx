#include <algorithm>
using std::sort;
using std::upper_bound;

#include <cmath>
using std::isnan;

#include <fstream>
using std::ofstream;
using std::ifstream;
using std::ios;

#include <limits>
using std::numeric_limits;

#include <iomanip>
using std::setw;
using std::setprecision;
using std::fixed;
using std::left;
using std::right;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::istream;
using std::hexfloat;
using std::defaultfloat;

#include <random>
using std::minstd_rand0;

#include <sstream>
using std::istringstream;
using std::ostringstream;

#include <string>
using std::string;
using std::to_string;

#include <vector>
using std::vector;


#ifdef _MYSQL_
#include "common/db_conn.hxx"
#endif

#include "common/random.hxx"
#include "common/exp.hxx"

#include "image_tools/image_set.hxx"
#include "cnn_node.hxx"
#include "cnn_edge.hxx"
#include "cnn_genome.hxx"

#include "stdint.h"


/**
 *  Initialize a genome from a file
 */
CNN_Genome::CNN_Genome(string filename, bool is_checkpoint) {
    exact_id = -1;
    genome_id = -1;
    started_from_checkpoint = is_checkpoint;

    ifstream infile(filename.c_str());
    read(infile);
    infile.close();
}

CNN_Genome::CNN_Genome(istream &in, bool is_checkpoint) {
    exact_id = -1;
    genome_id = -1;
    started_from_checkpoint = is_checkpoint;
    read(in);
}


void CNN_Genome::set_progress_function(int (*_progress_function)(double)) {
    progress_function = _progress_function;
}

int CNN_Genome::get_genome_id() const {
    return genome_id;
}

int CNN_Genome::get_exact_id() const {
    return exact_id;
}

double CNN_Genome::get_initial_mu() const {
    return initial_mu;
}

double CNN_Genome::get_mu() const {
    return mu;
}

double CNN_Genome::get_mu_delta() const {
    return mu_delta;
}

double CNN_Genome::get_initial_learning_rate() const {
    return initial_learning_rate;
}

double CNN_Genome::get_learning_rate() const {
    return learning_rate;
}

double CNN_Genome::get_learning_rate_delta() const {
    return learning_rate_delta;
}

double CNN_Genome::get_initial_weight_decay() const {
    return initial_weight_decay;
}

double CNN_Genome::get_weight_decay() const {
    return weight_decay;
}

double CNN_Genome::get_weight_decay_delta() const {
    return weight_decay_delta;
}

int CNN_Genome::get_velocity_reset() const {
    return velocity_reset;
}

double CNN_Genome::get_input_dropout_probability() const {
    return input_dropout_probability;
}

double CNN_Genome::get_hidden_dropout_probability() const {
    return hidden_dropout_probability;
}

template <class T>
void parse_array(vector<T> &output, istringstream &iss) {
    output.clear();

    T val;
    while(iss >> val || !iss.eof()) {
        if (iss.fail()) {
            iss.clear();
            string dummy;
            iss >> dummy;
            continue;
        }
        output.push_back(val);
        //cout << val << endl;
    }
}

#ifdef _MYSQL_
CNN_Genome::CNN_Genome(int _genome_id) {
    progress_function = NULL;

    ostringstream query;

    query << "SELECT * FROM cnn_genome WHERE id = " << _genome_id;

    //cout << query.str() << endl;

    mysql_exact_query(query.str());

    MYSQL_RES *result = mysql_store_result(exact_db_conn);

    if (result != NULL) {
        MYSQL_ROW row = mysql_fetch_row(result);

        genome_id = _genome_id; //this is also row[0]
        int column = 0;

        exact_id = atoi(row[++column]);

        vector<int> input_node_innovation_numbers;
        istringstream input_node_innovation_numbers_iss(row[++column]);
        //cout << "parsing input node innovation numbers" << endl;
        parse_array(input_node_innovation_numbers, input_node_innovation_numbers_iss);

        vector<int> softmax_node_innovation_numbers;
        istringstream softmax_node_innovation_numbers_iss(row[++column]);
        //cout << "parsing softmax node innovation numbers" << endl;
        parse_array(softmax_node_innovation_numbers, softmax_node_innovation_numbers_iss);

        istringstream generator_iss(row[++column]);
        generator_iss >> generator;

        istringstream normal_distribution_iss(row[++column]);
        normal_distribution_iss >> normal_distribution;

        //cout << "generator: " << generator << endl;

        velocity_reset = atoi(row[++column]);

        input_dropout_probability = atof(row[++column]);
        hidden_dropout_probability = atof(row[++column]);

        initial_mu = atof(row[++column]);
        mu = atof(row[++column]);
        mu_delta = atof(row[++column]);

        initial_learning_rate = atof(row[++column]);
        learning_rate = atof(row[++column]);
        learning_rate_delta = atof(row[++column]);

        initial_weight_decay = atof(row[++column]);
        weight_decay = atof(row[++column]);
        weight_decay_delta = atof(row[++column]);

        epoch = atoi(row[++column]);
        max_epochs = atoi(row[++column]);
        reset_weights = atoi(row[++column]);

        best_error = atof(row[++column]);
        best_error_epoch = atoi(row[++column]);
        best_predictions = atoi(row[++column]);
        best_predictions_epoch = atoi(row[++column]);

        istringstream best_class_error_iss(row[++column]);
        //cout << "parsing best class error" << endl;
        parse_array(best_class_error, best_class_error_iss);

        istringstream best_correct_predictions_iss(row[++column]);
        //cout << "parsing best correct predictions" << endl;
        parse_array(best_correct_predictions, best_correct_predictions_iss);

        started_from_checkpoint = atoi(row[++column]);

        backprop_order.clear();

        generation_id = atoi(row[++column]);
        name = row[++column];
        checkpoint_filename = row[++column];
        output_filename = row[++column];

        generated_by_disable_edge = atoi(row[++column]);
        generated_by_enable_edge = atoi(row[++column]);
        generated_by_split_edge = atoi(row[++column]);
        generated_by_add_edge = atoi(row[++column]);
        generated_by_change_size = atoi(row[++column]);
        generated_by_change_size_x = atoi(row[++column]);
        generated_by_change_size_y = atoi(row[++column]);
        generated_by_crossover = atoi(row[++column]);
        generated_by_reset_weights = atoi(row[++column]);
        generated_by_add_node = atoi(row[++column]);

        ostringstream node_query;
        node_query << "SELECT id FROM cnn_node WHERE genome_id = " << genome_id;
        //cout << node_query.str() << endl;

        mysql_exact_query(node_query.str());
        //cout << "node query was successful!" << endl;

        MYSQL_RES *node_result = mysql_store_result(exact_db_conn);
        //cout << "got result!" << endl;

        MYSQL_ROW node_row;
        while ((node_row = mysql_fetch_row(node_result)) != NULL) {
            int node_id = atoi(node_row[0]);
            //cout << "got node with id: " << node_id << endl;

            CNN_Node *node = new CNN_Node(node_id);
            nodes.push_back(node);

            if (find(input_node_innovation_numbers.begin(), input_node_innovation_numbers.end(), node->get_innovation_number()) != input_node_innovation_numbers.end()) {
                input_nodes.push_back(node);
            }

            if (find(softmax_node_innovation_numbers.begin(), softmax_node_innovation_numbers.end(), node->get_innovation_number()) != softmax_node_innovation_numbers.end()) {
                softmax_nodes.push_back(node);
            }
        }

        //cout << "got all nodes!" << endl;
        mysql_free_result(node_result);

        ostringstream edge_query;
        edge_query << "SELECT id FROM cnn_edge WHERE genome_id = " << genome_id;

        mysql_exact_query(edge_query.str());
        //cout << "edge query was successful!" << endl;

        MYSQL_RES *edge_result = mysql_store_result(exact_db_conn);
        //cout << "got result!" << endl;

        MYSQL_ROW edge_row;
        while ((edge_row = mysql_fetch_row(edge_result)) != NULL) {
            int edge_id = atoi(edge_row[0]);
            //cout << "got edge with id: " << edge_id << endl;

            CNN_Edge *edge = new CNN_Edge(edge_id);
            edges.push_back(edge);

            edge->set_nodes(nodes);
        }

        //cout << "got all edges!" << endl;
        mysql_free_result(edge_result);

        mysql_free_result(result);

    } else {
        cout << "Could not find genome with id: " << genome_id << "!" << endl;
        exit(1);
    }

    if (epoch > 0) {
        //if this was saved at an epoch > 0, it has already been initialized
        started_from_checkpoint = true;
    }
}

void CNN_Genome::export_to_database(int _exact_id) {
    exact_id = _exact_id;

    ostringstream query;

    if (genome_id >= 0) {
        query << "REPLACE INTO cnn_genome SET id = " << genome_id << ",";
    } else {
        query << "INSERT INTO cnn_genome SET";
    }

    query << " exact_id = " << exact_id
        << ", input_node_innovation_numbers = '";

    for (uint32_t i = 0; i < input_nodes.size(); i++) {
        if (i != 0) query << " ";
        query << input_nodes[i]->get_innovation_number();
    }

    query << "', softmax_node_innovation_numbers = '";

    for (uint32_t i = 0; i < softmax_nodes.size(); i++) {
        if (i != 0) query << " ";
        query << softmax_nodes[i]->get_innovation_number();
    }


    query << "', generator = '" << generator << "'"
        << ", normal_distribution = '" << normal_distribution << "'"
        << ", velocity_reset = '" << velocity_reset << "'"
        << ", input_dropout_probability = " << setprecision(15) << fixed << input_dropout_probability
        << ", hidden_dropout_probability = " << setprecision(15) << fixed << hidden_dropout_probability
        << ", initial_mu = " << setprecision(15) << fixed << initial_mu
        << ", mu = " << setprecision(15) << fixed<< mu
        << ", mu_delta = " << setprecision(15) << fixed << mu_delta
        << ", initial_learning_rate = " << setprecision(15) << fixed << initial_learning_rate
        << ", learning_rate = " << setprecision(15) << fixed << learning_rate
        << ", learning_rate_delta = " << setprecision(15) << fixed << learning_rate_delta
        << ", initial_weight_decay = " << setprecision(15) << fixed << initial_weight_decay
        << ", weight_decay = " << setprecision(15) << fixed<< weight_decay
        << ", weight_decay_delta = " << setprecision(15) << fixed << weight_decay_delta
        << ", epoch = " << epoch
        << ", max_epochs = " << max_epochs
        << ", reset_weights = " << reset_weights
        << ", best_error = " << setprecision(15) << fixed << best_error
        << ", best_predictions = " << best_predictions
        << ", best_predictions_epoch = " << best_predictions_epoch
        << ", best_error_epoch = " << best_error_epoch
        << ", best_class_error = '";

    for (uint32_t i = 0; i < best_class_error.size(); i++) {
        if (i != 0) query << " ";
        query << setprecision(15) << fixed << best_class_error[i];
    }

    query << "', best_correct_predictions = '";
    for (uint32_t i = 0; i < best_correct_predictions.size(); i++) {
        if (i != 0) query << " ";
        query << best_correct_predictions[i];
    }

    query << "', started_from_checkpoint = " << started_from_checkpoint;

    //too much overhead for saving this and no use for it
    //query << ", backprop_order = ''";
    /*
    query << ", backprop_order = '";
    for (uint32_t i = 0; i < backprop_order.size(); i++) {
        if (i != 0) query << " ";
        query << setprecision(15) << backprop_order[i];
    }
    query << "'";
    */

    query << ", generation_id = " << generation_id
        << ", name = '" << name << "'"
        << ", checkpoint_filename = '" << checkpoint_filename << "'"
        << ", output_filename = '" << output_filename << "'"
        << ", generated_by_disable_edge = " << generated_by_disable_edge
        << ", generated_by_enable_edge = " << generated_by_enable_edge
        << ", generated_by_split_edge = " << generated_by_split_edge
        << ", generated_by_add_edge = " << generated_by_add_edge
        << ", generated_by_change_size = " << generated_by_change_size
        << ", generated_by_change_size_x = " << generated_by_change_size_x
        << ", generated_by_change_size_y = " << generated_by_change_size_y
        << ", generated_by_crossover = " << generated_by_crossover
        << ", generated_by_reset_weights = " << generated_by_reset_weights
        << ", generated_by_add_node = " << generated_by_add_node;

    //cout << "query:\n" << query.str() << endl;

    mysql_exact_query(query.str());

    if (genome_id < 0) {
        genome_id = mysql_exact_last_insert_id(); //get last insert id from database
        cout << "setting genome id to: " << genome_id << endl;
    }

    for (uint32_t i = 0; i < nodes.size(); i++) {
        nodes[i]->export_to_database(exact_id, genome_id);
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        edges[i]->export_to_database(exact_id, genome_id);
    }
}

#endif

/**
 *  Iniitalize a genome from a set of nodes and edges
 */
CNN_Genome::CNN_Genome(int _generation_id, int seed, int _max_epochs, bool _reset_weights, int _velocity_reset, double _mu, double _mu_delta, double _learning_rate, double _learning_rate_delta, double _weight_decay, double _weight_decay_delta, double _input_dropout_probability, double _hidden_dropout_probability, const vector<CNN_Node*> &_nodes, const vector<CNN_Edge*> &_edges) {
    exact_id = -1;
    genome_id = -1;
    started_from_checkpoint = false;
    generator = minstd_rand0(seed);

    progress_function = NULL;

    velocity_reset = _velocity_reset;

    input_dropout_probability = _input_dropout_probability;
    hidden_dropout_probability = _hidden_dropout_probability;

    mu = _mu;
    initial_mu = mu;
    mu_delta = _mu_delta;

    learning_rate = _learning_rate;
    initial_learning_rate = learning_rate;
    learning_rate_delta = _learning_rate_delta;

    weight_decay = _weight_decay;
    initial_weight_decay = weight_decay;
    weight_decay_delta = _weight_decay_delta;

    epoch = 0;
    max_epochs = _max_epochs;
    reset_weights = _reset_weights;

    best_predictions = 0;
    best_error = EXACT_MAX_DOUBLE;

    best_predictions_epoch = 0;
    best_error_epoch = 0;

    generation_id = _generation_id;

    generated_by_disable_edge = 0;
    generated_by_enable_edge = 0;
    generated_by_split_edge = 0;
    generated_by_add_edge = 0;
    generated_by_change_size = 0;
    generated_by_change_size_x = 0;
    generated_by_change_size_y = 0;
    generated_by_crossover = 0;
    generated_by_reset_weights = 0;
    generated_by_add_node = 0;

    name = "";
    output_filename = "";
    checkpoint_filename = "";

    nodes.clear();
    input_nodes.clear();
    softmax_nodes.clear();

    for (uint32_t i = 0; i < _nodes.size(); i++) {
        CNN_Node *node_copy = _nodes[i]->copy();

        if (node_copy->is_input()) {
            //cout << "node was input!" << endl;

            input_nodes.push_back(node_copy);
        }

        if (node_copy->is_softmax()) {
            //cout << "node was softmax!" << endl;

            softmax_nodes.push_back(node_copy);
        }

        nodes.push_back( node_copy );
    }

    for (uint32_t i = 0; i < _edges.size(); i++) {
        CNN_Edge *edge_copy = _edges[i]->copy();

        if (!edge_copy->set_nodes(nodes)) {
            cerr << "ERROR: filter size didn't match when creating genome!" << endl;
            cerr << "This should never happen!" << endl;
            exit(1);
        }
        edges.push_back( edge_copy );
    }
}


CNN_Genome::~CNN_Genome() {
    while (nodes.size() > 0) {
        CNN_Node *node = nodes.back();
        nodes.pop_back();

        delete node;
    }

    while (edges.size() > 0) {
        CNN_Edge *edge = edges.back();
        edges.pop_back();

        delete edge;
    }
    
    while (input_nodes.size() > 0) {
        input_nodes.pop_back();
    }

    while (softmax_nodes.size() > 0) {
        softmax_nodes.pop_back();
    }
    softmax_nodes.clear();
}

bool CNN_Genome::equals(CNN_Genome *other) const {
    for (int32_t i = 0; i < (int32_t)edges.size(); i++) {
        CNN_Edge *edge = edges[i];

        if (edge->is_disabled()) continue;

        bool found = false;

        for (int32_t j = 0; j < other->get_number_edges(); j++) {
            CNN_Edge *other_edge = other->get_edge(j);

            if (other_edge->is_disabled()) continue;

            if (edge->get_innovation_number() == other_edge->get_innovation_number()) {
                found = true;

                if (!edge->equals(other_edge)) return false;
            }
        }

        if (!found) return false;
    }

    //other may have edges not in this genome, need to check this as well

    for (int32_t i = 0; i < other->get_number_edges(); i++) {
        CNN_Edge *other_edge = other->get_edge(i);

        if (other_edge->is_disabled()) continue;

        bool found = false;
        
        for (int32_t j = 0; j < (int32_t)edges.size(); j++) {
            CNN_Edge* edge = edges[j];

            if (edge->is_disabled()) continue;

            if (edge->get_innovation_number() == other_edge->get_innovation_number()) {
                found = true;
            }
        }

        if (!found) return false;
    }

    return true;
}

void CNN_Genome::print_best_error(ostream &out) const {
    cout << left << setw(20) << "class error:" << right;
    for (uint32_t i = 0; i < best_class_error.size(); i++) {
        cout << setw(15) << setprecision(5) << best_class_error[i];
    }
    cout << endl;
}

void CNN_Genome::print_best_predictions(ostream &out) const {
    cout << left << setw(20) << "correct predictions:" << right;
    for (uint32_t i = 0; i < best_correct_predictions.size(); i++) {
        cout << setw(15) << setprecision(5) << best_correct_predictions[i];
    }
    cout << endl;
}

int CNN_Genome::get_number_weights() const {
    int number_weights = 0;

    for (uint32_t i = 0; i < edges.size(); i++) {
        number_weights += edges[i]->get_filter_x() * edges[i]->get_filter_y();
    }

    return number_weights;
}

int CNN_Genome::get_number_biases() const {
    int number_biases = 0;

    for (uint32_t i = 0; i < nodes.size(); i++) {
        number_biases += nodes[i]->get_size_x() * nodes[i]->get_size_y();
    }

    return number_biases;
}

int CNN_Genome::get_operations_estimate() const {
    int operations_estimate = 0;

    for (uint32_t i = 0; i < nodes.size(); i++) {
        operations_estimate += nodes[i]->get_size_x() * nodes[i]->get_size_y();
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        bool reverse_filter_x = edges[i]->is_reverse_filter_x();
        bool reverse_filter_y = edges[i]->is_reverse_filter_y();

        if (reverse_filter_x && reverse_filter_y) {
            operations_estimate += edges[i]->get_filter_x() * edges[i]->get_filter_y() * edges[i]->get_input_node()->get_size_x() * edges[i]->get_input_node()->get_size_y();
        } else if (reverse_filter_x) {
            operations_estimate += edges[i]->get_filter_x() * edges[i]->get_filter_y() * edges[i]->get_input_node()->get_size_x() * edges[i]->get_output_node()->get_size_y();
        } else if (reverse_filter_y) {
            operations_estimate += edges[i]->get_filter_x() * edges[i]->get_filter_y() * edges[i]->get_output_node()->get_size_x() * edges[i]->get_input_node()->get_size_y();
        } else {
            operations_estimate += edges[i]->get_filter_x() * edges[i]->get_filter_y() * edges[i]->get_output_node()->get_size_x() * edges[i]->get_output_node()->get_size_y();
        }
    }

    return operations_estimate;
}


int CNN_Genome::get_generation_id() const {
    return generation_id;
}

double CNN_Genome::get_fitness() const {
    return best_error;
}

int CNN_Genome::get_max_epochs() const {
    return max_epochs;
}

int CNN_Genome::get_epoch() const {
    return epoch;
}

int CNN_Genome::get_best_error_epoch() const {
    return best_error_epoch;
}

int CNN_Genome::get_best_predictions() const {
    return best_predictions;
}

int CNN_Genome::get_number_enabled_edges() const {
    int number_enabled_edges = 0;

    for (uint32_t i = 0; i < edges.size(); i++) {
        if (!edges[i]->is_disabled()) number_enabled_edges++;
    }

    return number_enabled_edges;
}


const vector<CNN_Node*> CNN_Genome::get_nodes() const {
    return nodes;
}

const vector<CNN_Edge*> CNN_Genome::get_edges() const {
    return edges;
}

CNN_Node* CNN_Genome::get_node(int node_position) {
    return nodes.at(node_position);
}

CNN_Edge* CNN_Genome::get_edge(int edge_position) {
    return edges.at(edge_position);
}

int CNN_Genome::get_number_edges() const {
    return edges.size();
}

int CNN_Genome::get_number_nodes() const {
    return nodes.size();
}

int CNN_Genome::get_number_softmax_nodes() const {
    return softmax_nodes.size();
}

int CNN_Genome::get_number_input_nodes() const {
    return input_nodes.size();
}

void CNN_Genome::add_node(CNN_Node* node) {
    nodes.insert( upper_bound(nodes.begin(), nodes.end(), node, sort_CNN_Nodes_by_depth()), node );

}

void CNN_Genome::add_edge(CNN_Edge* edge) {
    edges.insert( upper_bound(edges.begin(), edges.end(), edge, sort_CNN_Edges_by_depth()), edge );
}

bool CNN_Genome::disable_edge(int edge_position) {
    CNN_Edge *edge = edges.at(edge_position);
    
    /*
    int number_inputs = edge->get_output_node()->get_number_inputs();

    if (number_inputs == 1) {
        return false;
    } else if (number_inputs == 0) {
        cerr << "ERROR: disabling an edge where the target had 0 inputs." << endl;
        cerr << "\tThis should never happen!" << endl;
        exit(1);
    }
    */

    if (edge->is_disabled()) {
        cout << "\t\tcould not disable edge " << edge_position << " because it was already disabled!" << endl;
        return true;
    } else {
        cout << "\t\tdisabling edge: " << edge_position << endl;
        edge->disable();
        return true;
    }
}

void CNN_Genome::resize_edges_around_node(int node_innovation_number) {
    for (uint32_t i = 0; i < edges.size(); i++) {
        CNN_Edge *edge = edges[i];

        if (edge->get_input_node()->get_innovation_number() == node_innovation_number) {
            cout << "\tresizing edge with innovation number " << edge->get_innovation_number() << " as input to node with innovation number " << node_innovation_number << endl;
            edge->resize();
        }

        if (edge->get_output_node()->get_innovation_number() == node_innovation_number) {
            cout << "\tresizing edge with innovation number " << edge->get_innovation_number() << " as output from node with innovation number " << node_innovation_number << endl;
            edge->resize();
        }
    }
}

bool CNN_Genome::sanity_check(int type) {
    //check to see if all edge filters are the correct size
    for (uint32_t i = 0; i < edges.size(); i++) {
        if (!edges[i]->is_filter_correct()) {
            cerr << "SANITY CHECK FAILED! edges[" << i << "] had incorrect filter size!" << endl;
            cerr << edges[i] << endl;
            return false;
        }
    }

    //check for duplicate edges, make sure edge size is sane
    for (uint32_t i = 0; i < edges.size(); i++) {
        if (edges[i]->get_filter_x() <= 0 || edges[i]->get_filter_x() > 100) {
            cerr << "ERROR: edge failed sanity check, reached impossible filter_x (<= 0 or > 100)" << endl;
            cerr << "edge in position " << i << " with innovation number: " << edges[i]->get_innovation_number() << endl;
            cerr << "filter_x: " << edges[i]->get_filter_x() << ", filter_y: " << edges[i]->get_filter_y() << endl;
            return false;
        }

        if (edges[i]->get_filter_y() <= 0 || edges[i]->get_filter_y() > 100) {
            cerr << "ERROR: edge failed sanity check, reached impossible filter_y (<= 0 or > 100)" << endl;
            cerr << "edge in position " << i << " with innovation number: " << edges[i]->get_innovation_number() << endl;
            cerr << "filter_x: " << edges[i]->get_filter_x() << ", filter_y: " << edges[i]->get_filter_y() << endl;
            return false;
        }

        for (uint32_t j = i + 1; j < edges.size(); j++) {
            if (edges[i]->get_innovation_number() == edges[j]->get_innovation_number()) {
                cerr << "SANITY CHECK FAILED! edges[" << i << "] and edges[" << j << "] have the same innovation number: " << edges[i]->get_innovation_number() << endl;
                return false;
            }
        }
    }

    //check for duplicate nodes, make sure node size is sane
    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->get_size_x() <= 0 || nodes[i]->get_size_x() > 100) {
            cerr << "ERROR: node failed sanity check, reached impossible size_x (<= 0 or > 100)" << endl;
            cerr << "node in position " << i << " with innovation number: " << nodes[i]->get_innovation_number() << endl;
            cerr << "size_x: " << nodes[i]->get_size_x() << ", size_y: " << nodes[i]->get_size_y() << endl;
            return false;
        }

        if (nodes[i]->get_size_y() <= 0 || nodes[i]->get_size_y() > 100) {
            cerr << "ERROR: node failed sanity check, reached impossible size_y (<= 0 or > 100)" << endl;
            cerr << "node in position " << i << " with innovation number: " << nodes[i]->get_innovation_number() << endl;
            cerr << "size_x: " << nodes[i]->get_size_x() << ", size_y: " << nodes[i]->get_size_y() << endl;
            return false;
        }

        for (uint32_t j = i + 1; j < nodes.size(); j++) {
            if (nodes[i]->get_innovation_number() == nodes[j]->get_innovation_number()) {
                cerr << "SANITY CHECK FAILED! nodes[" << i << "] and nodes[" << j << "] have the same innovation number: " << nodes[i]->get_innovation_number() << endl;
                return false;
            }
        }
    }

    if (type == SANITY_CHECK_AFTER_GENERATION) {
        for (uint32_t i = 0; i < nodes.size(); i++) {
            if (nodes[i]->has_zero_bias()) {
                cerr << "WARNING after generation!" << endl;
                cerr << "node in position " << i << " with innovation number: " << nodes[i]->get_innovation_number() << endl;
                cerr << "size_x: " << nodes[i]->get_size_x() << ", size_y: " << nodes[i]->get_size_y() << endl;
                cerr << "sum of bias was 0" << endl;
                nodes[i]->initialize_bias(generator, normal_distribution);
                nodes[i]->save_best_bias();
                //return false;
            }
        }
        //cout << "passed checking zero best bias" << endl;

        for (uint32_t i = 0; i < edges.size(); i++) {
            if (edges[i]->has_zero_weight()) {
                cerr << "WARNING before after_generation!" << endl;
                cerr << "edge in position " << i << " with innovation number: " << edges[i]->get_innovation_number() << endl;
                cerr << "filter_x: " << edges[i]->get_filter_x() << ", filter_y: " << edges[i]->get_filter_y() << endl;
                cerr << "sum of weights was 0" << endl;
                edges[i]->initialize_weights(generator, normal_distribution);
                edges[i]->save_best_weights();
                //return false;
            }
        }
        //cout << "passed checking zero best weights" << endl;

    } else if (type == SANITY_CHECK_BEFORE_INSERT) {
        //seems bias can go to 0
        /*
        for (uint32_t i = 0; i < nodes.size(); i++) {
            if (nodes[i]->has_zero_best_bias()) {
                cerr << "ERROR before insert!" << endl;
                cerr << "node in position " << i << " with innovation number: " << nodes[i]->get_innovation_number() << endl;
                cerr << "sum of best bias was 0" << endl;
                cerr << "size_x: " << nodes[i]->get_size_x() << ", size_y: " << nodes[i]->get_size_y() << endl;
                return false;
            }
        }
        */
        //cout << "passed checking zero best weights" << endl;

        /*
        for (uint32_t i = 0; i < edges.size(); i++) {
            if (edges[i]->has_zero_best_weight()) {
                cerr << "ERROR before insert!" << endl;
                cerr << "ERROR: edge in position " << i << " with innovation number: " << edges[i]->get_innovation_number() << endl;
                cerr << "sum of best weights was 0" << endl;
                cerr << "filter_x: " << edges[i]->get_filter_x() << ", filter_y: " << edges[i]->get_filter_y() << endl;
                return false;
            }
        }
        */
        //cout << "passed checking zero best weights" << endl;
    }


    //check to see if total_inputs on each node are correct (equal to non-disabled input edges)
    for (uint32_t i = 0; i < nodes.size(); i++) {
        int number_inputs = nodes[i]->get_number_inputs();

        //cout << "\t\tcounting inputs for node " << i << " (innovation number: " << nodes[i]->get_innovation_number() << ") -- should find " << number_inputs << endl;

        int counted_inputs = 0;
        for (uint32_t j = 0; j < edges.size(); j++) {
            if (edges[j]->is_disabled()) {
                //cout << "\t\t\tedge " << j << " is disabled (" << edges[j]->get_input_innovation_number() << " to " << edges[j]->get_output_innovation_number() << ")" << endl;
                continue;
            }

            if (edges[j]->get_output_node()->get_innovation_number() == nodes[i]->get_innovation_number()) {
                //cout << "\t\t\tedge " << j << " (" << edges[j]->get_input_innovation_number() << " to " << edges[j]->get_output_innovation_number() << ") output matches node innovation number" << endl;

                if (edges[j]->get_output_node() != nodes[i]) {
                    //these should be equal
                    cerr << "SANITY CHECK FAILED! edges[" << j << "]->output_node had the same innovation number as nodes[" << j << "] but the pointers were not the same!" << endl;
                    cerr << "EDGE[" << j << "]: " << endl;
                    cerr << edges[j] << endl << endl;
                    cerr << "NODE[" << i << "]: " << endl;
                    cerr << nodes[i] << endl << endl;
                    return false;
                }
                counted_inputs++;
            } else {
                //cout << "\t\t\tedge " << j << " (" << edges[j]->get_input_innovation_number() << " to " << edges[j]->get_output_innovation_number() << ") output does not match node innovation number" << endl;
            }
        }

        if (counted_inputs != number_inputs) {
            cerr << "SANITY CHECK FAILED! nodes[" << i << "] had total inputs: " << number_inputs << " but " << counted_inputs << " inputs were counted. " << endl;
            cerr << "node innovation number: " << nodes[i]->get_innovation_number() << endl;
            for (uint32_t j = 0; j < edges.size(); j++) {
                if (edges[j]->get_output_node()->get_innovation_number() == nodes[i]->get_innovation_number()) {
                    cerr << "\tedge with innovation number " << edges[j]->get_innovation_number() << " had node as output, edge disabled? " << edges[j]->is_disabled() << endl;
                }
            }
            return false;
        }
    }

    return true;
}

bool CNN_Genome::outputs_connected() const {
    //check to see there is a path to from the input to each output

    for (uint32_t i = 0; i < nodes.size(); i++) {
        nodes[i]->set_unvisited();
    }

    for (uint32_t i = 0; i < input_nodes.size(); i++) {
        input_nodes[i]->visit();
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        if (!edges[i]->is_disabled()) {
            if (edges[i]->get_input_node()->is_visited()) {
                edges[i]->get_output_node()->visit();
            }
        }
    }

    for (uint32_t i = 0; i < softmax_nodes.size(); i++) {
        if (!softmax_nodes[i]->is_visited()) {
            return false;
        }
    }

    return true;
}

int CNN_Genome::evaluate_image(const Image &image, vector<double> &class_error, bool perform_backprop, bool perform_dropout, double &total_error) {
    int expected_class = image.get_classification();
    int rows = image.get_rows();
    int cols = image.get_cols();

    for (uint32_t i = 0; i < nodes.size(); i++) {
        nodes[i]->reset();
    }

    for (uint32_t channel = 0; channel < input_nodes.size(); channel++) {
        input_nodes[channel]->set_values(image, channel, rows, cols, perform_dropout, generator, input_dropout_probability);
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        edges[i]->propagate_forward(perform_dropout, generator, hidden_dropout_probability);
    }

    //cout << "before softmax max: ";
    double softmax_max = softmax_nodes[0]->get_value(0,0);
    //cout << " " << setw(15) << fixed << setprecision(6) << softmax_nodes[0]->get_value(0,0);

    for (uint32_t i = 1; i < softmax_nodes.size(); i++) {
        //cout << " " << setw(15) << fixed << setprecision(6) << softmax_nodes[i]->get_value(0,0);
        if (softmax_nodes[i]->get_value(0,0) > softmax_max) {
            softmax_max = softmax_nodes[i]->get_value(0,0);
        }
    }
    //cout << endl;

    //cout << "after softmax max:  ";
    double softmax_sum = 0.0;
    for (uint32_t i = 0; i < softmax_nodes.size(); i++) {
        double value = softmax_nodes[i]->get_value(0,0);
        double previous = value;

        if (isnan(value)) {
            cerr << "ERROR: value was NAN before exp!" << endl;
            exit(1);
        }

        value = exact_exp(value - softmax_max);

        //cout << " " << setw(15) << fixed << setprecision(6) << value;
        if (isnan(value)) {
            cerr << "ERROR: value was NAN AFTER exp! previously: " << previous << endl;
            exit(1);
        }

        softmax_nodes[i]->set_value(0, 0, value);
        //cout << "\tvalue " << softmax_nodes[i]->get_innovation_number() << ": " << softmax_nodes[i]->get_value(0,0) << endl;
        softmax_sum += value;

        if (isnan(softmax_sum)) {
            cerr << "ERROR: softmax_sum was NAN AFTER add!" << endl;
            exit(1);
        }
    }
    //cout << endl;

    if (softmax_sum == 0) {
        cout << "ERROR! softmax sum == 0" << endl;
        exit(1);
    }

    //cout << "softmax sum: " << softmax_sum << endl;

    double max_value = -numeric_limits<double>::max();
    int predicted_class = -1;

    //cout << "error:          ";
    for (int32_t i = 0; i < (int32_t)softmax_nodes.size(); i++) {
        double value = softmax_nodes[i]->get_value(0,0) / softmax_sum;
        //cout << "\tvalue " << softmax_nodes[i]->get_innovation_number() << ": " << softmax_nodes[i]->get_value(0,0) << endl;

        if (isnan(value)) {
            cerr << "ERROR: value was NAN AFTER divide by softmax_sum, previously: " << softmax_nodes[i]->get_value(0,0) << endl;
            cerr << "softmax_sum: " << softmax_sum << endl;
            exit(1);
        }

        softmax_nodes[i]->set_value(0, 0,  value);

        //softmax_nodes[i]->print(cout);

        int target = 0.0;
        if (i == expected_class) {
            target = 1.0;
        }
        double error = value - target;
        double gradient = value * (1 - value);
        //cout << "\t" << softmax_nodes[i]->get_innovation_number() << " -- value: " << value << ", error: " << error << ", gradient: " << gradient << endl;

        softmax_nodes[i]->set_error(0, 0, error);
        softmax_nodes[i]->set_gradient(0, 0, gradient);

        class_error[i] += fabs(error);


        if (value > max_value) {
            predicted_class = i;
            max_value = value;
        }

        total_error -= target * log(value);
        //total_error += fabs(error);


        //cout << " " << setw(15) << fixed << setprecision(6) << error;
    }
    //cout << endl;
    //cout << "predicted class: " << predicted_class << endl;
    //cout << "expected class:  " << expected_class << endl;

    if (perform_backprop) {
        for (int32_t i = edges.size() - 1; i >= 0; i--) {
            edges[i]->propagate_backward();
        }

        for (int32_t i = 0; i < edges.size(); i++) {
            edges[i]->update_weights(mu, learning_rate, weight_decay);
        }

        for (int32_t i = 0; i < nodes.size(); i++) {
            nodes[i]->propagate_bias(mu, learning_rate, weight_decay);
        }
    }

    return predicted_class;
}

void CNN_Genome::save_to_best() {
    for (uint32_t i = 0; i < edges.size(); i++) {
        edges[i]->save_best_weights();
    }

    for (uint32_t i = 0; i < nodes.size(); i++) {
        nodes[i]->save_best_bias();
    }
}

void CNN_Genome::set_to_best() {
    for (uint32_t i = 0; i < edges.size(); i++) {
        edges[i]->set_weights_to_best();
    }

    for (uint32_t i = 0; i < nodes.size(); i++) {
        nodes[i]->set_bias_to_best();
    }
}

void CNN_Genome::initialize() {
    cout << "initializing genome!" << endl;
    for (uint32_t i = 0; i < nodes.size(); i++) {
        nodes[i]->reset_weight_count();
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        edges[i]->propagate_weight_count();
    }
    cout << "calculated weight counts" << endl;

    if (reset_weights) {
        for (uint32_t i = 0; i < edges.size(); i++) {
            edges[i]->initialize_weights(generator, normal_distribution);
            edges[i]->save_best_weights();
        }
        cout << "initialized weights!" << endl;

        for (uint32_t i = 0; i < nodes.size(); i++) {
            nodes[i]->initialize_bias(generator, normal_distribution);
            nodes[i]->save_best_bias();
        }
        cout << "initialized bias!" << endl;
    } else {
        for (uint32_t i = 0; i < edges.size(); i++) {
            if (edges[i]->needs_init()) {
                edges[i]->initialize_weights(generator, normal_distribution);
                edges[i]->save_best_weights();
            }
        }
        cout << "reinitialized weights!" << endl;

        for (uint32_t i = 0; i < nodes.size(); i++) {
            if (nodes[i]->needs_init()) {
                nodes[i]->initialize_bias(generator, normal_distribution);
                nodes[i]->save_best_bias();
            }
        }
        cout << "reinitialized bias!" << endl;
        set_to_best();

        /*
        for (uint32_t i = 0; i < edges.size(); i++) {
            if (edges[i]->has_zero_weight()) {
                cerr << "ERROR: edge in position " << i << " with innovation number: " << edges[i]->get_innovation_number() << endl;
                cerr << "sum of weights was 0" << endl;
                cerr << "filter_x: " << edges[i]->get_filter_x() << ", filter_y: " << edges[i]->get_filter_y() << endl;
                exit(1);
            }
        }
        */
    }
}

void CNN_Genome::print_progress(ostream &out, int total_predictions, double total_error) const {
    out << "[" << setw(10) << name << ", genome " << setw(5) << generation_id << "] predictions: " << setw(7) << total_predictions << ", best: " << setw(7) << best_predictions << "/" << backprop_order.size() << " (" << setw(5) << fixed << setprecision(2) << (100 * (double)best_predictions/(double)backprop_order.size()) << "%), error: " << setw(15) << setprecision(5) << fixed << total_error << ", best error: " << setw(15) << best_error << " on epoch: " << setw(5) << best_error_epoch << ", epoch: " << setw(4) << epoch << "/" << max_epochs << ", mu: " << setw(12) << fixed << setprecision(10) << mu << ", learning_rate: " << setw(12) << fixed << setprecision(10) << learning_rate << ", weight_decay: " << setw(12) << fixed << setprecision(10) << weight_decay << endl;
}


void CNN_Genome::evaluate(const Images &images, vector<double> &class_error, vector<int> &correct_predictions, double &total_error, int &total_predictions, bool perform_backprop) {
    class_error.assign(images.get_number_classes(), 0.0);
    correct_predictions.assign(images.get_number_classes(), 0);

    bool perform_dropout;
    if (perform_backprop) {
        perform_dropout = true;
    } else {
        perform_dropout = false;
    }

    total_error = 0.0;
    total_predictions = 0;
    for (uint32_t j = 0; j < backprop_order.size(); j++) {
        int predicted_class = evaluate_image(images.get_image(backprop_order[j]), class_error, perform_backprop, perform_dropout, total_error);
        int expected_class = images.get_image(backprop_order[j]).get_classification();
        //cout << "predicted class: " << predicted_class << ", expected class: " << expected_class << endl;

        if (perform_backprop && velocity_reset > 0 && j > 0 && ((j % velocity_reset) == 0)) {
            for (uint32_t i = 0; i < edges.size(); i++) {
                edges[i]->reset_velocities();
            }

            for (uint32_t i = 0; i < nodes.size(); i++) {
                nodes[i]->reset_velocities();
            }
        }

        /*
           if (j < 5) {
           cerr << "class error: ";
           for (uint32_t i = 0; i < class_error.size(); i++) {
           cerr << setw(13) << setprecision(5) << fixed << class_error[i];
           }
           cerr << endl;
           }
           */

        if (predicted_class == expected_class) {
            correct_predictions[expected_class]++;
            total_predictions++;
        }
    }

    /*
    for (uint32_t j = 0; j < class_error.size(); j++) {
        total_error += class_error[j];
    }
    */
}


void CNN_Genome::evaluate(const Images &images, double &total_error, int &total_predictions) {
    vector<double> class_error;
    vector<int> correct_predictions;

    backprop_order.clear();
    for (int32_t i = 0; i < images.get_number_images(); i++) {
        backprop_order.push_back(i);
    }

    evaluate(images, class_error, correct_predictions, total_error, total_predictions, false);

    print_progress(cerr, total_predictions, total_error);
}

void CNN_Genome::stochastic_backpropagation(const Images &images) {
    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->needs_init()) {
            cerr << "ERROR! nodes[" << i << "] needs init!" << endl;
            exit(1);
        }

        if (nodes[i]->has_nan()) {
            cerr << "ERROR! nodes[" << i << "] has nan or inf!" << endl;
            nodes[i]->print(cerr);
            exit(1);
        }
    }

    for (uint32_t i = 0; i < edges.size(); i++) {
        if (edges[i]->needs_init()) {
            cerr << "ERROR! edges[" << i << "] needs init!" << endl;
            exit(1);
        }

        if (edges[i]->has_nan()) {
            cerr << "ERROR! edges[" << i << "] has nan or inf!" << endl;
            edges[i]->print(cerr);
            exit(1);
        }
    }

    if (!started_from_checkpoint) {
        backprop_order.clear();
        for (int32_t i = 0; i < images.get_number_images(); i++) {
            backprop_order.push_back(i);
        }

        cerr << "generator min: " << generator.min() << ", generator max: " << generator.max() << endl;

        cerr << "pre shuffle 1: " << generator() << endl;

        //shuffle the array (thanks C++ not being the same across operating systems)
        fisher_yates_shuffle(generator, backprop_order);

        cerr << "post shuffle 1: " << generator() << endl;

        best_error = EXACT_MAX_DOUBLE;
    }
    backprop_order.resize(2000);

    //sort edges by depth of input node
    sort(edges.begin(), edges.end(), sort_CNN_Edges_by_depth());

    vector<int> class_sizes(images.get_number_classes(), 0);
    for (uint32_t i = 0; i < backprop_order.size(); i++) {
        class_sizes[ images.get_image(backprop_order[i]).get_classification() ]++;
    }

    double total_error = 0.0;
    int total_predictions = 0;
    vector<double> class_error(images.get_number_classes(), 0.0);
    vector<int> correct_predictions(images.get_number_classes(), 0);

    bool evaluate_initial_weights = true;
    if  (evaluate_initial_weights) {
        evaluate(images, class_error, correct_predictions, total_error, total_predictions, false);
        print_progress(cerr, total_predictions, total_error);
    }

    do {
        //shuffle the array (thanks C++ not being the same across operating systems)
        fisher_yates_shuffle(generator, backprop_order);

        evaluate(images, class_error, correct_predictions, total_error, total_predictions, true);
        evaluate(images, class_error, correct_predictions, total_error, total_predictions, false);

        bool found_improvement = false;
        if (total_error < best_error) {
            best_error = total_error;
            best_error_epoch = epoch;
            best_predictions = total_predictions;
            best_predictions_epoch = epoch;

            best_class_error = class_error;
            best_correct_predictions = correct_predictions;

            if (output_filename.compare("") != 0) {
                write_to_file(output_filename);
            }

            save_to_best();
            found_improvement = true;
        }
        print_progress(cerr, total_predictions, total_error);

        /*
        if (total_error > 100000 && total_error != EXACT_MAX_DOUBLE) {
            for (uint32_t i = 0; i < edges.size(); i++) {
                edges[i]->print_statistics();
            }

            for (uint32_t i = 0; i < nodes.size(); i++) {
                nodes[i]->print_statistics();
            }
        }
        */

        if (!found_improvement) {
            set_to_best();
        }

        //decay mu with a max of 0.99
        mu = 0.99 - ((0.99 - mu) * mu_delta);

        learning_rate *= learning_rate_delta;
        //if (learning_rate < 0.00001) learning_rate = 0.00001;

        weight_decay *= weight_decay_delta;
        //if (weight_decay < 0.00001) weight_decay = 0.00001;

        epoch++;

        if (checkpoint_filename.compare("") != 0) {
            write_to_file(checkpoint_filename);
        }

        if (progress_function != NULL) {
            double progress = (double)epoch / (double)(max_epochs + 1.0);
            progress_function(progress);
        }

        if (epoch > max_epochs) {
            break;
        }
    } while (true);
}

void CNN_Genome::set_name(string _name) {
    name = _name;
}

void CNN_Genome::set_output_filename(string _output_filename) {
    output_filename = _output_filename;
}

void CNN_Genome::set_checkpoint_filename(string _checkpoint_filename) {
    checkpoint_filename = _checkpoint_filename;
}

string CNN_Genome::get_version_str() const {
    return version_str;
}

#ifdef _WIN32
#define EXACT_VERSION "0.19"
#endif

void CNN_Genome::write(ostream &outfile) {
    outfile << "v" << EXACT_VERSION << endl;
    outfile << exact_id << endl;
    outfile << genome_id << endl;

    write_hexfloat(outfile, initial_mu);
    outfile << endl;
    write_hexfloat(outfile, mu);
    outfile << endl;
    write_hexfloat(outfile, mu_delta);
    outfile << endl;

    write_hexfloat(outfile, initial_learning_rate);
    outfile << endl;
    write_hexfloat(outfile, learning_rate);
    outfile << endl;
    write_hexfloat(outfile, learning_rate_delta);
    outfile << endl;

    write_hexfloat(outfile, initial_weight_decay);
    outfile << endl;
    write_hexfloat(outfile, weight_decay);
    outfile << endl;
    write_hexfloat(outfile, weight_decay_delta);
    outfile << endl;

    write_hexfloat(outfile, input_dropout_probability);
    outfile << endl;
    write_hexfloat(outfile, hidden_dropout_probability);
    outfile << endl;

    outfile << velocity_reset << endl;

    outfile << epoch << endl;
    outfile << max_epochs << endl;
    outfile << reset_weights << endl;

    outfile << best_predictions << endl;
    write_hexfloat(outfile, best_error);
    outfile << endl;

    outfile << best_predictions_epoch << endl;
    outfile << best_error_epoch << endl;

    outfile << generated_by_disable_edge << endl;
    outfile << generated_by_enable_edge << endl;
    outfile << generated_by_split_edge << endl;
    outfile << generated_by_add_edge << endl;
    outfile << generated_by_change_size << endl;
    outfile << generated_by_change_size_x << endl;
    outfile << generated_by_change_size_y << endl;
    outfile << generated_by_crossover << endl;
    outfile << generated_by_reset_weights << endl;
    outfile << generated_by_add_node << endl;

    outfile << generation_id << endl;
    outfile << normal_distribution << endl;
    //outfile << name << endl;
    //outfile << checkpoint_filename << endl;
    //outfile << output_filename << endl;

    outfile << generator << endl;

    outfile << "NODES" << endl;
    outfile << nodes.size() << endl;
    for (uint32_t i = 0; i < nodes.size(); i++) {
        outfile << nodes[i] << endl;
    }

    outfile << "EDGES" << endl;
    outfile << edges.size() << endl;
    for (uint32_t i = 0; i < edges.size(); i++) {
        outfile << edges[i] << endl;
    }

    outfile << "INNOVATION_NUMBERS" << endl;
    outfile << input_nodes.size() << endl;
    for (uint32_t i = 0; i < input_nodes.size(); i++) {
        outfile << input_nodes[i]->get_innovation_number() << endl;
    }

    outfile << softmax_nodes.size() << endl;
    for (uint32_t i = 0; i < softmax_nodes.size(); i++) {
        outfile << softmax_nodes[i]->get_innovation_number() << endl;
    }

    outfile << "BACKPROP_ORDER" << endl;
    outfile << backprop_order.size() << endl;
    for (uint32_t i = 0; i < backprop_order.size(); i++) {
        if (i > 0) outfile << " ";
        outfile << backprop_order[i];
    }
    outfile << endl;

    outfile << "BEST_CLASS_ERROR" << endl;
    outfile << best_class_error.size() << endl;
    for (uint32_t i = 0; i < best_class_error.size(); i++) {
        if (i > 0) outfile << " ";
        outfile << best_class_error[i];
    }
    outfile << endl;

    outfile << "BEST_CORRECT_PREDICTIONS" << endl;
    outfile << best_correct_predictions.size() << endl;
    for (uint32_t i = 0; i < best_correct_predictions.size(); i++) {
        if (i > 0) outfile << " ";
        outfile << best_correct_predictions[i];
    }
    outfile << endl;
}

void CNN_Genome::read(istream &infile) {
    progress_function = NULL;

    bool verbose = true;

    getline(infile, version_str);

    cerr << "read CNN_Genome file with version string: '" << version_str << "'" << endl;

    if (version_str.substr(1,4).compare(EXACT_VERSION) != 0) {
        cerr << "breaking because version_str '" << version_str.substr(1,4) << "' did not match EXACT_VERSION '" << EXACT_VERSION << "': " << version_str.compare(EXACT_VERSION) << endl;
        return;
    }

    infile >> exact_id;
    if (verbose) cerr << "read exact_id: " << exact_id << endl;
    infile >> genome_id;
    if (verbose) cerr << "read genome_id: " << genome_id << endl;

    initial_mu = read_hexfloat(infile);
    if (verbose) cerr << "read initial_mu: " << initial_mu << endl;
    mu = read_hexfloat(infile);
    if (verbose) cerr << "read mu: " << mu << endl;
    mu_delta = read_hexfloat(infile);
    if (verbose) cerr << "read mu_delta: " << mu_delta << endl;

    initial_learning_rate = read_hexfloat(infile);
    if (verbose) cerr << "read initial_learning_rate: " << initial_learning_rate << endl;
    learning_rate = read_hexfloat(infile);
    if (verbose) cerr << "read learning_rate: " << learning_rate << endl;
    learning_rate_delta = read_hexfloat(infile);
    if (verbose) cerr << "read learning_rate_delta: " << learning_rate_delta << endl;

    initial_weight_decay = read_hexfloat(infile);
    if (verbose) cerr << "read initial_weight_decay: " << initial_weight_decay << endl;
    weight_decay = read_hexfloat(infile);
    if (verbose) cerr << "read weight_decay: " << weight_decay << endl;
    weight_decay_delta = read_hexfloat(infile);
    if (verbose) cerr << "read weight_decay_delta: " << weight_decay_delta << endl;

    input_dropout_probability = read_hexfloat(infile);
    if (verbose) cerr << "read input_dropout_probability: " << input_dropout_probability << endl;
    hidden_dropout_probability = read_hexfloat(infile);
    if (verbose) cerr << "read hidden_dropout_probability: " << hidden_dropout_probability << endl;

    infile >> velocity_reset;
    if (verbose) cerr << "read velocity_reset: " << velocity_reset << endl;

    infile >> epoch;
    if (verbose) cerr << "read epoch: " << epoch << endl;
    infile >> max_epochs;
    if (verbose) cerr << "read max_epochs: " << max_epochs << endl;
    infile >> reset_weights;
    if (verbose) cerr << "read reset_weights: " << reset_weights << endl;

    infile >> best_predictions;
    if (verbose) cerr << "read best_predictions: " << best_predictions << endl;
    best_error = read_hexfloat(infile);
    if (verbose) cerr << "read best_error: " << best_error << endl;
    infile >> best_predictions_epoch;
    if (verbose) cerr << "read best_predictions_epoch: " << best_predictions_epoch << endl;
    infile >> best_error_epoch;
    if (verbose) cerr << "read best_error_epoch: " << best_error_epoch << endl;

    infile >> generated_by_disable_edge;
    if (verbose) cerr << "read generated_by_disable_edge: " << generated_by_disable_edge << endl;
    infile >> generated_by_enable_edge;
    if (verbose) cerr << "read generated_by_enable_edge: " << generated_by_enable_edge << endl;
    infile >> generated_by_split_edge;
    if (verbose) cerr << "read generated_by_split_edge: " << generated_by_split_edge << endl;
    infile >> generated_by_add_edge;
    if (verbose) cerr << "read generated_by_add_edge: " << generated_by_add_edge << endl;
    infile >> generated_by_change_size;
    if (verbose) cerr << "read generated_by_change_size: " << generated_by_change_size << endl;
    infile >> generated_by_change_size_x;
    if (verbose) cerr << "read generated_by_change_size_x: " << generated_by_change_size_x << endl;
    infile >> generated_by_change_size_y;
    if (verbose) cerr << "read generated_by_change_size_y: " << generated_by_change_size_x << endl;
    infile >> generated_by_crossover;
    if (verbose) cerr << "read generated_by_crossover: " << generated_by_crossover << endl;
    infile >> generated_by_reset_weights;
    if (verbose) cerr << "read generated_by_reset_weights: " << generated_by_reset_weights << endl;
    infile >> generated_by_add_node;
    if (verbose) cerr << "read generated_by_add_node: " << generated_by_add_node << endl;

    infile >> generation_id;
    if (verbose) cerr << "read generation_id: " << generation_id << endl;
    //infile >> name;
    //infile >> checkpoint_filename;
    //infile >> output_filename;

    infile >> normal_distribution;
    if (verbose) cerr << "read normal distribution: '" << normal_distribution << "'" << endl;

    //for some reason linux doesn't read the generator correcly because of
    //the first newline
    string generator_str;
    getline(infile, generator_str);
    getline(infile, generator_str);
    if (verbose) cerr << "generator_str: '" << generator_str << "'" << endl;
    istringstream generator_iss(generator_str);
    generator_iss >> generator;
    //infile >> generator;

    if (verbose) {
        cerr << "read generator: " << generator << endl;
        //cerr << "rand 1: " << generator() << endl;
        //cerr << "rand 2: " << generator() << endl;
        //cerr << "rand 3: " << generator() << endl;
    }

    //cerr << "reading nodes!" << endl;
    
    string line;
    getline(infile, line);

    if (line.compare("NODES") != 0) {
        cerr << "ERROR: invalid input file, expected line to be 'NODES' but line was '" << line << "'" << endl;
        version_str = "INALID";
        return;
    }

    nodes.clear();
    int number_nodes;
    infile >> number_nodes;
    //cerr << "reading " << number_nodes << " nodes." << endl;
    for (int32_t i = 0; i < number_nodes; i++) {
        CNN_Node *node = new CNN_Node();
        infile >> node;

        //cerr << "read node: " << node->get_innovation_number() << endl;
        nodes.push_back(node);
    }

    //cerr << "reading edges!" << endl;

    getline(infile, line);
    getline(infile, line);
    if (line.compare("EDGES") != 0) {
        cerr << "ERROR: invalid input file, expected line to be 'EDGES' but line was '" << line << "'" << endl;
        version_str = "INALID";
        return;
    }

    edges.clear();
    int number_edges;
    infile >> number_edges;
    //cerr << "reading " << number_edges << " edges." << endl;
    for (int32_t i = 0; i < number_edges; i++) {
        CNN_Edge *edge = new CNN_Edge();
        infile >> edge;

        //cerr << "read edge: " << edge->get_innovation_number() << endl;
        if (!edge->set_nodes(nodes)) {
            cerr << "ERROR: filter size didn't match when reading genome from input file!" << endl;
            cerr << "This should never happen!" << endl;
            exit(1);
        }

        edges.push_back(edge);
    }

    getline(infile, line);
    getline(infile, line);
    if (line.compare("INNOVATION_NUMBERS") != 0) {
        cerr << "ERROR: invalid input file, expected line to be 'INNOVATION_NUMBERS' but line was '" << line << "'" << endl;
        version_str = "INALID";
        return;
    }

    input_nodes.clear();
    int number_input_nodes;
    infile >> number_input_nodes;
    //cerr << "number input nodes: " << number_input_nodes << endl;

    for (int32_t i = 0; i < number_input_nodes; i++) {
        int input_node_innovation_number;
        infile >> input_node_innovation_number;
        //cerr << "\tinput node: " << input_node_innovation_number << endl;

        for (uint32_t i = 0; i < nodes.size(); i++) {
            if (nodes[i]->get_innovation_number() == input_node_innovation_number) {
                input_nodes.push_back(nodes[i]);
                //cerr << "input node " << input_node_innovation_number << " was in position: " << i << endl;
                break;
            }
        }
    }

    softmax_nodes.clear();
    int number_softmax_nodes;
    infile >> number_softmax_nodes;
    //cerr << "number softmax nodes: " << number_softmax_nodes << endl;

    for (int32_t i = 0; i < number_softmax_nodes; i++) {
        int softmax_node_innovation_number;
        infile >> softmax_node_innovation_number;
        //cerr << "\tsoftmax node: " << softmax_node_innovation_number << endl;

        for (uint32_t i = 0; i < nodes.size(); i++) {
            if (nodes[i]->get_innovation_number() == softmax_node_innovation_number) {
                softmax_nodes.push_back(nodes[i]);
                //cerr << "softmax node " << softmax_node_innovation_number << " was in position: " << i << endl;
                break;
            }
        }
    }

    //cerr << "reading backprop order" << endl;

    getline(infile, line);
    getline(infile, line);
    if (line.compare("BACKPROP_ORDER") != 0) {
        cerr << "ERROR: invalid input file, expected line to be 'BACKPROP_ORDER' but line was '" << line << "'" << endl;
        version_str = "INALID";
        return;
    }

    backprop_order.clear();
    long order_size;
    infile >> order_size;
    //cout << "order_size: " << order_size << endl;
    for (uint32_t i = 0; i < order_size; i++) {
        long order;
        infile >> order;
        backprop_order.push_back(order);
        //cerr << "backprop order[" << i << "]: " << order << endl;
    }


    //cerr << "reading best class error" << endl;

    if (order_size == 0) getline(infile, line);
    getline(infile, line);
    getline(infile, line);
    if (line.compare("BEST_CLASS_ERROR") != 0) {
        cerr << "ERROR: invalid input file, expected line to be 'BEST_CLASS_ERROR' but line was '" << line << "'" << endl;
        version_str = "INALID";
        return;
    }

    best_class_error.clear();
    int error_size;
    infile >> error_size;
    //cerr << "error_size: " << error_size << endl;
    for (int32_t i = 0; i < error_size; i++) {
        double error;
        infile >> error;
        best_class_error.push_back(error);
    }

    //cerr << "reading best correct predictions" << endl;

    if (error_size == 0) getline(infile, line);
    getline(infile, line);
    getline(infile, line);
    if (line.compare("BEST_CORRECT_PREDICTIONS") != 0) {
        cerr << "ERROR: invalid input file, expected line to be 'BEST_CORRECT_PREDICTIONS' but line was '" << line << "'" << endl;
        version_str = "INALID";
        return;
    }

    best_correct_predictions.clear();
    int predictions_size;
    infile >> predictions_size;
    for (int32_t i = 0; i < predictions_size; i++) {
        double predictions;
        infile >> predictions;
        best_correct_predictions.push_back(predictions);
    }
}

void CNN_Genome::write_to_file(string filename) {
    ofstream outfile(filename.c_str());
    write(outfile);
    outfile.close();
}


void CNN_Genome::print_graphviz(ostream &out) const {
    out << "digraph CNN {" << endl;

    //this will draw graph left to right instead of top to bottom
    //out << "\trankdir=LR;" << endl;

    //print the source nodes, i.e. the input
    out << "\t{" << endl;
    out << "\t\trank = source;" << endl;
    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (!nodes[i]->is_input()) continue;
        out << "\t\tnode" << nodes[i]->get_innovation_number() << " [shape=box,color=green,label=\"input " << nodes[i]->get_innovation_number() << "\\n" << nodes[i]->get_size_x() << " x " << nodes[i]->get_size_y() << "\"];" << endl;
    }
    out << "\t}" << endl;
    out << endl;

    out << "\t{" << endl;
    out << "\t\trank = sink;" << endl;
    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (!nodes[i]->is_softmax()) continue;
        out << "\t\tnode" << nodes[i]->get_innovation_number() << " [shape=box,color=blue,label=\"output " << (nodes[i]->get_innovation_number() - 1) << "\\n" << nodes[i]->get_size_x() << " x " << nodes[i]->get_size_y() << "\"];" << endl;
    }
    out << "\t}" << endl;
    out << endl;

    //connect the softmax nodes in order with invisible edges so they display in order

    bool printed_first = false;

    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (!nodes[i]->is_softmax()) continue;

        if (!printed_first) {
            printed_first = true;
            out << "\tnode" << nodes[i]->get_innovation_number();
        } else {
            out << " -> node" << nodes[i]->get_innovation_number();
        }
    }
    out << " [style=invis];" << endl << endl;

    out << endl;

    //draw the hidden nodes
    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->is_input() || nodes[i]->is_softmax()) continue;

        out << "\t\tnode" << nodes[i]->get_innovation_number() << " [shape=box,label=\"input " << nodes[i]->get_innovation_number() << "\\n" << nodes[i]->get_size_x() << " x " << nodes[i]->get_size_y() << "\"];" << endl;
    }
    
    out << endl;

    //draw the enabled edges
    for (uint32_t i = 0; i < edges.size(); i++) {
        if (!edges[i]->is_disabled()) {
            out << "\tnode" << edges[i]->get_input_node()->get_innovation_number() << " -> node" << edges[i]->get_output_node()->get_innovation_number() << ";" << endl;
        }
    }

    out << endl;

    /*
    //draw the disabled edges in red
    for (uint32_t i = 0; i < edges.size(); i++) {
        if (edges[i]->is_disabled()) {
            out << "\tnode" << edges[i]->get_input_node()->get_innovation_number() << " -> node" << edges[i]->get_output_node()->get_innovation_number() << " [color=red];" << endl;
        }
    }
    */

    out << "}" << endl;
}

void CNN_Genome::set_generated_by_disable_edge() {
    generated_by_disable_edge++;
}

void CNN_Genome::set_generated_by_enable_edge() {
    generated_by_enable_edge++;
}

void CNN_Genome::set_generated_by_split_edge() {
    generated_by_split_edge++;
}

void CNN_Genome::set_generated_by_add_edge() {
    generated_by_add_edge++;
}

void CNN_Genome::set_generated_by_change_size() {
    generated_by_change_size++;
}

void CNN_Genome::set_generated_by_change_size_x() {
    generated_by_change_size_x++;
}

void CNN_Genome::set_generated_by_change_size_y() {
    generated_by_change_size_y++;
}

void CNN_Genome::set_generated_by_crossover() {
    generated_by_crossover++;
}

void CNN_Genome::set_generated_by_reset_weights() {
    generated_by_reset_weights++;
}

void CNN_Genome::set_generated_by_add_node() {
    generated_by_add_node++;
}



int CNN_Genome::get_generated_by_disable_edge() {
    return generated_by_disable_edge;
}

int CNN_Genome::get_generated_by_enable_edge() {
    return generated_by_enable_edge;
}

int CNN_Genome::get_generated_by_split_edge() {
    return generated_by_split_edge;
}

int CNN_Genome::get_generated_by_add_edge() {
    return generated_by_add_edge;
}

int CNN_Genome::get_generated_by_change_size() {
    return generated_by_change_size;
}

int CNN_Genome::get_generated_by_change_size_x() {
    return generated_by_change_size_x;
}

int CNN_Genome::get_generated_by_change_size_y() {
    return generated_by_change_size_y;
}

int CNN_Genome::get_generated_by_crossover() {
    return generated_by_crossover;
}

int CNN_Genome::get_generated_by_reset_weights() {
    return generated_by_reset_weights;
}

int CNN_Genome::get_generated_by_add_node() {
    return generated_by_add_node;
}

