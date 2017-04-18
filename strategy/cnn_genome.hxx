#ifndef CNN_GENOME_H
#define CNN_GENOME_H

#include <random>
using std::minstd_rand0;

#include <vector>
using std::vector;

#include "image_tools/image_set.hxx"
#include "cnn_node.hxx"
#include "cnn_edge.hxx"
#include "common/random.hxx"

#define SANITY_CHECK_BEFORE_INSERT 0
#define SANITY_CHECK_AFTER_GENERATION 1

//mysql can't handl the max double value for some reason
#define EXACT_MAX_DOUBLE 10000000

class CNN_Genome {
    private:
        string version_str;
        int exact_id;
        int genome_id;

        vector<CNN_Node*> nodes;
        vector<CNN_Edge*> edges;

        vector<CNN_Node*> input_nodes;
        vector<CNN_Node*> softmax_nodes;

        NormalDistribution normal_distribution;
        minstd_rand0 generator;

        int velocity_reset;

        int batch_size;
        double epsilon;
        double alpha;

        double input_dropout_probability;
        double hidden_dropout_probability;

        double initial_mu;
        double mu;
        double mu_delta;

        double initial_learning_rate;
        double learning_rate;
        double learning_rate_delta;

        double initial_weight_decay;
        double weight_decay;
        double weight_decay_delta;

        int epoch;
        int max_epochs;
        bool reset_weights;

        double best_error;
        int best_predictions;
        int best_predictions_epoch;
        int best_error_epoch;

        bool started_from_checkpoint;
        vector<long> backprop_order;

        int generation_id;
        string name;
        string checkpoint_filename;
        string output_filename;

        int generated_by_disable_edge;
        int generated_by_enable_edge;
        int generated_by_split_edge;
        int generated_by_add_edge;
        int generated_by_change_size;
        int generated_by_change_size_x;
        int generated_by_change_size_y;
        int generated_by_crossover;
        int generated_by_reset_weights;
        int generated_by_add_node;

        int (*progress_function)(double);

    public:
        /**
         *  Initialize a genome from a file
         */
        CNN_Genome(string filename, bool is_checkpoint);
        CNN_Genome(istream &in, bool is_checkpoint);

        /**
         *  Iniitalize a genome from a set of nodes and edges
         */
        CNN_Genome(int _generation_id, int seed, int _max_epochs, bool _reset_weights, int velocity_reset, double _mu, double _mu_delta, double _learning_rate, double _learning_rate_delta, double _weight_decay, double _weight_decay_delta, int _batch_size, double _epsilon, double _alpha, double _input_dropout_probability, double _hidden_dropout_probability, const vector<CNN_Node*> &_nodes, const vector<CNN_Edge*> &_edges);

        ~CNN_Genome();

#ifdef _MYSQL_
        CNN_Genome(int genome_id);
        void export_to_database(int exact_id);
#endif

        double get_version() const;
        string get_version_str() const;


        int get_genome_id() const;
        int get_exact_id() const;

        bool equals(CNN_Genome *other) const;

        void print_progress(ostream &out, double total_error, int correct_predictions) const;

        int get_number_weights() const;
        int get_number_biases() const;

        int get_operations_estimate() const;

        void set_progress_function(int (*_progress_function)(double));

        int get_generation_id() const;
        double get_fitness() const;
        int get_best_error_epoch() const;
        int get_best_predictions() const;
        int get_epoch() const;
        int get_max_epochs() const;
        int get_number_enabled_edges() const;

        bool sanity_check(int type);
        bool outputs_connected() const;

        const vector<CNN_Node*> get_nodes() const;
        const vector<CNN_Edge*> get_edges() const;

        CNN_Node* get_node(int node_position);
        CNN_Edge* get_edge(int edge_position);

        double get_initial_mu() const;
        double get_mu() const;
        double get_mu_delta() const;
        double get_initial_learning_rate() const;
        double get_learning_rate() const;
        double get_learning_rate_delta() const;
        double get_initial_weight_decay() const;
        double get_weight_decay() const;
        double get_weight_decay_delta() const;

        double get_alpha() const;
        int get_velocity_reset() const;

        double get_input_dropout_probability() const;
        double get_hidden_dropout_probability() const;

        int get_number_edges() const;
        int get_number_nodes() const;
        int get_number_softmax_nodes() const;
        int get_number_input_nodes() const;

        void add_node(CNN_Node* node);
        void add_edge(CNN_Edge* edge);
        bool disable_edge(int edge_position);

        void resize_edges_around_node(int node_position);
 
        void evaluate_images(const vector<const Image> &images, bool training, double &total_error, int &correct_predictions);

        void set_to_best();
        void save_to_best();
        void initialize();

        void evaluate(const Images &images, double &total_error, int &correct_predictions);
        void evaluate(const Images &images, double &total_error, int &correct_predictions, bool perform_backprop);

        void stochastic_backpropagation(const Images &images);

        void set_name(string _name);
        void set_output_filename(string _output_filename);
        void set_checkpoint_filename(string _checkpoint_filename);

        void write(ostream &outfile);
        void write_to_file(string filename);

        void read(istream &infile);

        void print_graphviz(ostream &out) const;

        void set_generated_by_disable_edge();
        void set_generated_by_enable_edge();
        void set_generated_by_split_edge();
        void set_generated_by_add_edge();
        void set_generated_by_change_size();
        void set_generated_by_change_size_x();
        void set_generated_by_change_size_y();
        void set_generated_by_crossover();
        void set_generated_by_reset_weights();
        void set_generated_by_add_node();

        int get_generated_by_disable_edge();
        int get_generated_by_enable_edge();
        int get_generated_by_split_edge();
        int get_generated_by_add_edge();
        int get_generated_by_change_size();
        int get_generated_by_change_size_x();
        int get_generated_by_change_size_y();
        int get_generated_by_crossover();
        int get_generated_by_reset_weights();
        int get_generated_by_add_node();
};


struct sort_genomes_by_fitness {
    bool operator()(CNN_Genome *g1, CNN_Genome *g2) {
        return g1->get_fitness() < g2->get_fitness();
    }   
};

struct sort_genomes_by_predictions {
    bool operator()(CNN_Genome *g1, CNN_Genome *g2) {
        return g1->get_best_predictions() > g2->get_best_predictions();
    }   
};



#endif
