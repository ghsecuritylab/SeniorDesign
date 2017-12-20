function [Xest,P]=state_estimate_parameter(H,P,R,X,Y)
%[Xest,P]=state_estimate_parameter(F,H,P,R,Q,X,Y);
%the system transition matrix is identity so it is not an argument
%H is the measurement matrix
%P is the error covariance matrix
%R is the measurement noise covariance
%Q is R*eye(length(X))
%X is the previous state estimation
%Y is the current measurement
%returns updated error covariance matrix and the updated state estimation

const = eye(length(X));
Q = R*const;
% tic
x_p = X; %apriori estimation
M = P + Q; % apriori error covariance 
inter = M*H';
Gk = inter/((R+H*inter)); %Kalman gain
Xest = x_p+Gk*(Y-H*x_p); %updated state estimation
P = (const-Gk*H); %aposteriori error covariance
P = P*M;
% toc

end