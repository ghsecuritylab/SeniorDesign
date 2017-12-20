%% set parameters
s = 3; %top layer dimension
k = 10; %second layer dimension
varOut = 0.5; %noise parameter for observation equation
varState = 0.01; %noise parameter for state transition equations
noi = 0.01; %noise for test

locationToSaveResult = '.\'; %set to current folder in Matlab
epoch = 6; %how many times algorithm should go over all the notes
%% train the model
trainingVar2(s, k, varOut, varState, locationToSaveResult, epoch);

%% find cluster centers
centers = determine_clusters_with_self_transitions_var2(s, k, varOut, ...
                                                    varState, ...
                                                    locationToSaveResult);
                                                                                               
centerFilename = strcat(locationToSaveResult, ...
    ['centers_s' num2str(s) '_k' num2str(k) ...
    '_varOut' num2str(varOut) '_varState' num2str(varState) '.mat']);
save(centerFilename, 'centers')

%% test classification accuracy

[class_rate, class, true_label, SNR] = ...
    classification_given_centers_SNR_var2(centers, noi, s, k, ...
                                            varOut, varState, ...
                                            locationToSaveResult);
resultsFilename = strcat(locationToSaveResult, ...
    ['results_s' num2str(s) '_k' num2str(k) ...
    '_varOut' num2str(varOut) '_varState' num2str(varState) '.mat']);
save(resultsFilename, 'class_rate', 'class', 'true_label')

disp(['Achieved ' num2str(100*class_rate) ' % classification accuracy!'])