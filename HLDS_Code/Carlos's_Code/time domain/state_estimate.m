function [X_est,P] = state_estimate(F,H,P,R,Q,X,Y)
%[Xest,P] = state_estimate(F,H,P,R,Q,X,Y);
% F is the system transition matrix
% H is the measurement matrix
% P is the error covariance matrix
% R is the measurement noise covariance
% Q is the state transition noise covariance
% X is the previous state estimation
% Y is the current measurement
%returns updated error covariance matrix and the updated state estimation

% Prediction
X_p = F*X;                        % apriori estimation
P_k = F*P*F' + Q;                 % apriori error covariance

% Update 
K_k = P_k*H'/((H*P_k*H' + R));    % Kalman gain
X_est = X_p + K_k*(Y - H*X_p);    % updated state estimation
P = (eye(length(X)) - K_k*H)*P_k; % aposteriori error covariance

end