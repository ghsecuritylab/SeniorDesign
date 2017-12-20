function [Xest,P]=state_estimate(F,H,P,R,Q,X,Y)
%[Xest,P]=state_estimate(F,H,P,R,Q,X,Y);
%F is the system transition matrix
%H is the measurement matrix
%P is the error covariance matrix
%R is the measurement noise covariance
%Q is the state transition noise covariance
%X is the previous state estimation
%Y is the current measurement
%returns updated error covariance matrix and the updated state estimation

const = eye(length(X));
% tic
x_p=F*X; %apriori estimation
M= F*P*F' + Q; % apriori error covariance 
Gk=M*H'/((R+H*M*H')); %Kalman gain
Xest=x_p+Gk*(Y-H*x_p); %updated state estimation
P=(const-Gk*H)*M; %aposteriori error covariance
% toc

end