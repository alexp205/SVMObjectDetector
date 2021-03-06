#include "SVM_mgr.h"

std::string wstr_to_str(const std::wstring& wstr)
{
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    return converter.to_bytes(wstr);
}

int getTargetMap(std::string train_data_path, std::string dict_path)
{
    // setup
    // image file reading
    std::vector<std::string> files;
    DIR *img_dir;
    struct dirent *img_dir_rdr;

    // image processing and BoF
    cv::Ptr<cv::Feature2D> detector;
   
    // Step 1: read in images and labels
    std::wcout << L"Reading in picture(s)...\n";
    if (NULL == (img_dir = opendir(train_data_path.c_str()))) {
        std::wcout << L"error opening image directory\n";
        return -1;
    }

    while (NULL != (img_dir_rdr = readdir(img_dir))) {
        std::string img_fname = std::string(img_dir_rdr->d_name);
        if (img_fname.size() > 4) {

            // DEBUG
            //std::cout << "image filename: " << img_fname << "\n";

            files.push_back(img_fname);
        }
    }
    closedir(img_dir);

    int num_bags = 100;
    cv::TermCriteria tc(cv::TermCriteria::MAX_ITER, 100, 0.001);
    int retries = 1;
    int flags = cv::KMEANS_PP_CENTERS;

    detector = cv::xfeatures2d::SIFT::create();
    cv::BOWKMeansTrainer bof_trainer(num_bags, tc, retries, flags);

    for (int i = 0; i < files.size(); i++) {
        std::string img_fpath = train_data_path + "\\" + files[i];

        // DEBUG
        //std::cout << "image filepath: " << img_fpath << "\n";

        cv::Mat img = cv::imread(img_fpath, cv::IMREAD_GRAYSCALE);

        if (img.empty()) {
            std::wcout << L"invalid input\n";
            return -1;
        }

        // DEBUG
        //cv::imshow("image", img);
        //cv::waitKey(0);

        // Step 2:
        std::vector<cv::KeyPoint> keypts;
        cv::Mat descriptor_set;

        // extract SIFT features and get descriptors
        detector->detect(img, keypts);
        detector->compute(img, keypts, descriptor_set);
        bof_trainer.add(descriptor_set);

        // DEBUG
        /*cv::Mat descriptor_img;
        cv::drawKeypoints(img, keypts, descriptor_img, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
        cv::imshow("image", descriptor_img);
        cv::waitKey(0);
        cv::destroyWindow("image");*/

        std::wcout << L"Progress = " << ((static_cast<double>(i+1) / static_cast<double>(files.size())) * 100) << L"%\n";
    }

    // Step 3: train model to recognize descriptors/features
    cv::Mat kmeans_dict = bof_trainer.cluster();

    // DEBUG
    // (debug) view kmeans dict/vocab sample
    /*std::wcout << L"Row 0: [";
    for (int j = 0; j < kmeans_dict.cols; j++) {
        if (j != (kmeans_dict.cols - 1)) {
            std::wcout << kmeans_dict.at<float>(0, j) << ", ";
        }
        else {
            std::wcout << kmeans_dict.at<float>(0, j);
        }
    }
    std::wcout << "]\n";*/

    // Step 4: save model
    cv::FileStorage fs(dict_path.c_str(), cv::FileStorage::WRITE);
    fs << "vocabulary" << kmeans_dict;
    fs.release();
    
    return 0;
}

std::vector<std::vector<double>> getDescriptors(std::string image_dir_path, std::string dict_path)
{
    // setup
    std::vector<std::string> files;
    DIR *img_dir;
    struct dirent *img_dir_rdr;
    std::vector<std::vector<double>> descriptors;

    // image processing and BoF
    cv::Mat kmeans_dict;
    cv::Ptr<cv::DescriptorMatcher> matcher = cv::BFMatcher::create();
    cv::Ptr<cv::FeatureDetector> detector = cv::xfeatures2d::SIFT::create();
    cv::Ptr<cv::DescriptorExtractor> extractor = cv::xfeatures2d::SIFT::create();    // NOTE: same as detector, could be removed tbh imho tbph
    cv::BOWImgDescriptorExtractor bof_extractor(extractor, matcher);
    cv::FileStorage fs_out("C:\\Users\\ap\\Documents\\Projects\\Programs\\AI\\SVMImageClassifier\\histogram_descriptors.yml", cv::FileStorage::WRITE);

    std::wcout << L"Reading in picture(s)...\n";
    if (NULL == (img_dir = opendir(image_dir_path.c_str()))) {
        std::wcout << L"error opening image directory\n";
        return descriptors;
    }

    while (NULL != (img_dir_rdr = readdir(img_dir))) {
        std::string img_fname = std::string(img_dir_rdr->d_name);
        if (img_fname.size() > 4) {

            // DEBUG
            //std::cout << "image filename: " << img_fname << "\n";

            files.push_back(img_fname);
        }
    }
    closedir(img_dir);

    // set vocabulary
    cv::FileStorage fs(dict_path.c_str(), cv::FileStorage::READ);
    fs["vocabulary"] >> kmeans_dict;
    fs.release();

    bof_extractor.setVocabulary(kmeans_dict);

    // DEBUG
    // (debug) setup viewing of matched images
    /*std::string match_img_fpath = "C:\\Users\\ap\\Documents\\Projects\\Programs\\AI\\resources\\SVMImageClassifier\\match_img.jpg";
    std::vector<cv::KeyPoint> match_keypts;
    cv::Mat match_descriptor_set;
    cv::Mat match_img = cv::imread(match_img_fpath, cv::IMREAD_GRAYSCALE);
    detector->detect(match_img, match_keypts);
    // - (sub-debug) view keypts
    cv::Mat temp_img;
    cv::drawKeypoints(match_img, match_keypts, temp_img, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    cv::imshow("image", temp_img);
    cv::waitKey(0);
    cv::destroyWindow("image");
    // - (sub-debug) get NON-HISTOGRAM (i.e. actual image) descriptor
    extractor->compute(match_img, match_keypts, match_descriptor_set);*/

    for (int i = 0; i < files.size(); i++) {
        std::string img_fpath = image_dir_path + "\\" + files[i];

        // DEBUG
        //std::cout << "image filepath: " << img_fpath << "\n";

        cv::Mat img = cv::imread(img_fpath, cv::IMREAD_GRAYSCALE);

        if (img.empty()) {
            std::wcout << L"invalid input\n";
            return descriptors;
        }

        // DEBUG
        //cv::imshow("image", img);
        //cv::waitKey(0);

        std::vector<cv::KeyPoint> keypts;
        cv::Mat descriptor_set;

        // extract SIFT features and match w/ vocabulary
        detector->detect(img, keypts);
        bof_extractor.compute(img, keypts, descriptor_set);    // the BOWImgDescriptorExtractor computes a histogram descriptor, not the image descriptor

        // DEBUG
        // (debug) visualize histogram descriptor, NOTE: has a tendency to crash for large dicts/vocabs
        /*int nan_cnt = 0;
        int max_bin = 0;
        double max_val = 0;
        double avg_val = 0;
        for (int j = 0; j < descriptor_set.cols; j++) {
            double desc_val = descriptor_set.at<float>(0, j);
            if (std::isnan(desc_val)) {
                std::wcout << L"found nan @ " << j << L"\n";
                nan_cnt++;
            }
            else {
                std::wcout << L"Bin " << j << L": " << desc_val << L"\n";
                avg_val += desc_val;
            }
            if (desc_val > max_val) {
                max_val = desc_val;
                max_bin = j;
            }
        }
        avg_val = avg_val / (descriptor_set.cols + 1);
        std::wcout << L"max bin: " << max_bin << L", max value: " << max_val << L"\navg value: " << avg_val << L", # of nan values found: " << nan_cnt << L"\n";*/
        // (debug) get image descriptor for matching
        //cv::Mat test_descriptor_set;
        //extractor->compute(img, keypts, test_descriptor_set);    // this gives the image descriptor and should be used with match tests

        // detect and replace nan values w/ 0
        for (int j = 0; j < descriptor_set.cols; j++) {
            if (std::isnan(descriptor_set.at<float>(0, j))) descriptor_set.at<float>(0, j) = 0;
        }

        // DEBUG
        /*int nan_check = 0;
        for (int j = 0; j < descriptor_set.cols; j++) {
            if (std::isnan(descriptor_set.at<float>(0, j))) {
                std::wcout << L"nan detected @ " << j << L"\n";
                nan_check = 1;
            }
        }
        if (nan_check) {
            std::wcout << L"Mission failed, we'll get 'em next time.\n";
        }*/

        // reformat to SVM-training expectations
        std::vector<double> img_descriptor;
        int rows = descriptor_set.rows;
        int cols = descriptor_set.cols;
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                img_descriptor.push_back(descriptor_set.at<float>(r, c));
            }
        }
        descriptors.push_back(img_descriptor);
        fs_out << "img" << descriptor_set;

        // DEBUG
        /*std::wcout << L"# of match_img keypts: " << match_keypts.size() << L"\n# of test_img keypts: " << keypts.size() << L"\n";
        std::wcout << L"# of match_img descriptors: " << match_descriptor_set.rows << L"x" << match_descriptor_set.cols << L"\n";
        std::wcout << L"# of test_img descriptors: " << test_descriptor_set.rows << L"x" << test_descriptor_set.cols << L"\n";
        std::vector<std::vector<cv::DMatch>> matches;
        matcher->knnMatch(test_descriptor_set, match_descriptor_set, matches, 2);
        // (debug) filter out bad matches
        std::vector<cv::DMatch> good_matches;
        for (int j = 0; j < matches.size(); i++) {
            if (matches[j][0].distance < (0.7 * matches[j][1].distance)) {
                good_matches.push_back(matches[j][0]);
            }
        }
        std::wcout << L"# of matches = " << matches.size() << L"\n";    // for some reason (probably because of some of the matcher settings), every keypt is matched -> not very useful
        std::wcout << L"# of good matches = " << good_matches.size() << L"\n";
        cv::Mat good_matches_img;
        cv::drawMatches(img, keypts, match_img, match_keypts, good_matches, good_matches_img);
        //cv::drawMatches(img, keypts, match_img, match_keypts, matches, good_matches_img);    // this, at least w/ current settings, just shows every possible connection per above reason
        cv::imshow("good matches image", good_matches_img);
        cv::waitKey(0);
        cv::destroyWindow("good matches image");*/
        
        std::wcout << L"Progress = " << ((static_cast<double>(i + 1) / static_cast<double>(files.size())) * 100) << L"%\n";
    }

    fs_out.release();

    // DEBUG
    //std::wcout << L"image descriptors size: " << descriptors.size() << "\n";    // should just be the number of images used for training

    return descriptors;
}

int trainSVM(std::string image_dir_path, std::string train_labels_path, std::string dict_path, std::string model_path)
{
    // setup
    std::vector<std::vector<double>> img_descriptors;
    struct svm_problem prob;
    struct svm_parameter param;
    struct svm_model *model;

    img_descriptors = getDescriptors(image_dir_path, dict_path);

    // preprocess data
    prob.l = img_descriptors.size();
    prob.x = (struct svm_node**)malloc(sizeof(struct svm_node*) * prob.l);
    for (int i = 0; i < prob.l; i++) {
        prob.x[i] = (struct svm_node*)malloc(sizeof(struct svm_node) * (img_descriptors[0].size() + 1));
    }
    prob.y = (double*)malloc(sizeof(double) * prob.l);

    std::ifstream labels_file(train_labels_path);
    if (labels_file.fail()) {
        std::wcout << L"training labels file does not exist\n";
        return -1;
    }
    std::string label_str;
    int idx = 0;
    while (std::getline(labels_file, label_str)) {

        // DEBUG
        //std::cout << "label: " << label_str << "\n";

        prob.y[idx] = std::stoi(label_str, nullptr);
        idx++;
    }
    labels_file.close();

    int rows = img_descriptors.size();
    int cols = img_descriptors[0].size();
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            struct svm_node data;
            data.index = c;
            data.value = img_descriptors[r][c];
            prob.x[r][c] = data;
        }
        struct svm_node end_data;
        end_data.index = -1;
        end_data.value = 0.0;
        prob.x[r][cols] = end_data;
    }

    param.svm_type = C_SVC;
    param.kernel_type = RBF;
    param.degree = 3;
    param.gamma = 0.1;
    param.coef0 = 0;
    param.nu = 0.5;
    param.cache_size = 100;
    param.C = 0.0001;
    param.eps = 1e-3;
    param.p = 0.1;
    param.shrinking = 1;
    param.probability = 0;
    param.nr_weight = 0;
    param.weight_label = NULL;
    param.weight = NULL;

    // train SVM
    const char *error_msg = svm_check_parameter(&prob, &param);
    if (NULL != error_msg) {
        std::cerr << "Error in svm params: " << error_msg << "\n";
        return -1;
    }

    model = svm_train(&prob, &param);
    int success = svm_save_model(model_path.c_str(), model);
    if (0 != success) {
        std::wcerr << L"Error in saving model to file\n";
        return -1;
    }
    svm_free_and_destroy_model(&model);

    // clean up
    svm_destroy_param(&param);
    free(prob.y);
    for (int i = 0; i < prob.l; i++) {
        free(prob.x[i]);
    }
    free(prob.x);

    return 0;
}

std::vector<double> classifyImages(std::string test_data_path, std::string dict_path, std::string model_path)
{
    std::vector<std::vector<double>> img_descriptors;
    std::vector<double> classes;
    struct svm_model *model;

    img_descriptors = getDescriptors(test_data_path, dict_path);

    model = svm_load_model(model_path.c_str());
    if (0 == model) {
        std::wcerr << L"Error loading SVM model\n";
        return classes;
    }

    struct svm_node *data = (struct svm_node*)malloc(sizeof(struct svm_node) * (img_descriptors[0].size() + 1));

    int rows = img_descriptors.size();
    for (int img = 0; img < rows; img++) {
        for (int c = 0; c < img_descriptors[0].size(); c++) {
            struct svm_node img_data;
            img_data.index = c;
            img_data.value = img_descriptors[img][c];
            data[c] = img_data;
        }
        struct svm_node end_data;
        end_data.index = -1;
        end_data.value = 0.0;
        data[img_descriptors[0].size()] = end_data;

        // DEBUG
        /*for (int i = 0; i < (img_descriptors[0].size() + 1); i++) {
            std::wcout << L"data sample [" << i << L"] (check idx -> " << data[i].index << L") = " << data[i].value << L"\n";
        }*/

        double prediction = svm_predict(model, data);
        classes.push_back(prediction);
    }

    // clean up
    svm_free_and_destroy_model(&model);
    free(data);

    return classes;
}