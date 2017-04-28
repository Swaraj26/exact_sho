<?php

$cwd[__FILE__] = __FILE__;
if (is_link($cwd[__FILE__])) $cwd[__FILE__] = readlink($cwd[__FILE__]);
$cwd[__FILE__] = dirname($cwd[__FILE__]);

require_once($cwd[__FILE__] . "/../../citizen_science_grid/my_query.php");

$exact_result = query_exact_db("SELECT search_name FROM exact_search");

while ($exact_row = $exact_result->fetch_assoc()) {
    $search_name = $exact_row['search_name'];

    $progress_file = "/home/tdesell/exact/www/progress/" . $search_name . "_fitness_progress.png";

    $command = "python /home/tdesell/exact/visualization/plot_progress.py /projects/csg/exact_data/" . $search_name . "/progress.txt /home/tdesell/exact/www/progress/" . $search_name . "_fitness_progress.png /home/tdesell/exact/www/progress/" . $search_name . "_epochs_progress.png /home/tdesell/exact/www/progress/" . $search_name . "_generated_progress.png";

    $command2 = "python /home/tdesell/exact/visualization/plot_hyperparameters.py "
        . "/projects/csg/exact_data/" . $search_name . "/hyperparameters.txt "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_initial_mu.png "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_mu_delta.png "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_initial_learning_rate.png "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_learning_rate_delta.png "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_initial_weight_decay.png "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_weight_decay_delta.png "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_alpha.png "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_velocity_reset.png "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_input_dropout.png "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_hidden_dropout.png "
        . "/home/tdesell/exact/www/hyperparameters/" . $search_name . "_batch_size.png ";

    if (!file_exists($progress_file)) {
        echo "'$progress_file' does not exist!\n";
        echo "command: $command \n";
        echo "results: " . exec($command) . "\n";
        echo "command2: $command2 \n";
        echo "results2: " . exec($command2) . "\n";
    } else {
        $diff_time = time() - filemtime($progress_file);
        if ($diff_time > 300) {
            echo "REGENERATING IMAGES!\n";
            echo "command: $command \n";
            echo "results: " . exec($command) . "\n";
            echo "command2: $command2 \n";
            echo "results2: " . exec($command2) . "\n";
        } else {
            echo "SKIPPING '$search_name', progress files generated < 5 minutes ago!\n";
        }
    }
}

?>

