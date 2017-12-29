function centers = centers_notes(alph,se,win,H,F,B,G,D,C,P,h)

centers = zeros(3,15);          % Centers for 15 synthetic frequencies
% HDLS parameters
Fs = 11025;                       % sampling frequency
s = 3;                          % dimension of the third layer states (z)
k = 10;                         % dimension of the second layer states (u)
n = 60;                         % dimension of the first layer states (x) - also number of filters for this case
m = n;                          % dimension  of the observation (y)

% Joint state transition matrix
FJ = [C zeros(s,k) zeros(s,n); D G zeros(k,n) ; zeros(n,s) B,F]; 
% Joint measurement matrix
HJ = [zeros(m,s) zeros(m,k) H]; 

% Noise parameters
Q = 0.01*eye(k+n+s);             % process noise covariance matrix
R = 0.5;                        % measurement noise variance

% Filter bank
M = size(h,1);                       % Order of the filters

load('trumpet_iowa_cell.mat')
col =  collection(:,1:35);
for i = 1:35
    auxx1 = (col{i})';
    aux_idx1 = round(length(auxx1)/2);
    auxx1 = [auxx1(1:2000) auxx1(aux_idx1-2000:aux_idx1+2000) auxx1(length(auxx1)-2000:end)];              % length = 8002
    auxx1 = auxx1/(max(abs(auxx1)));
    musicM = zeros(34:2*length(auxx1));
    for j = 1:35
        ct = 1;
        if i ~= j
            ct = ct + 1;
            auxx2 = (col{j})';
            aux_idx2 = round(length(auxx2)/2);
            auxx2 = [auxx2(1:2000) auxx2(aux_idx2-2000:aux_idx2+2000) auxx2(length(auxx2)-2000:end)];              % length = 8002
            auxx2 = auxx2/(max(abs(auxx2)));
            musicM(ct,:) = [auxx1 auxx2];
        end
    end
    musicC{i} = musicM;
end

figure
if M >= win
    auxind = M;
else
    auxind = win;
end
for ind_C = 1:35
    disp(['Starting note ' num2str(ind_C)])
    mC = musicC{ind_C};
    for ind_M = 1:34
        musict = mC(ind_M,:);
        filtpp_v = zeros(n,win);
        IIR_v = zeros(n,win);
        prevIIR = zeros(n,1);
        X_v = zeros(n,win);
        U_v = zeros(k,win);
        Z_v = zeros(s,win);
        
        for i = auxind:length(musict)
            % Filtering - Inner product
            musicin = musict(i-M+1:i);                              % M samples for online filtering
            musict_v = musict(i-win+1:i);
            filtpp = zeros(n,1);
            % Filtering
            for j = 1:n
                filtpp(j,1) = musicin*(fliplr(h(:,j)));
            end
            filtpp_v = [filtpp_v(:,2:end) filtpp];
            filtpp = filtpp.^2;
            Y = alph*filtpp + (1-alph)*prevIIR;              % Observation - square or absolute value
            IIR_v = [IIR_v(:,2:end) Y];
            prevIIR = Y;
            
            % First iteration - state estimation only
            if i == auxind
                % Initial values of states: zeros
                [XJ,P] = state_estimate(FJ,HJ,P,R,Q,zeros(k+n+s,1),Y);
                Z = XJ(1:s);
                U = XJ(s+1:s+k);
                X = XJ(s+k+1:s+k+n);
                X_v = [X_v(:,2:end) X];
                U_v = [U_v(:,2:end) U];
                Z_v = [Z_v(:,2:end) Z];
            else
                % State estimation
                [XJ,P] = state_estimate(FJ,HJ,P,R,Q,XJ,Y);
                Z = XJ(1:s);
                U = XJ(s+1:s+k);
                X = XJ(s+k+1:s+k+n);
                X_v = [X_v(:,2:end) X];
                U_v = [U_v(:,2:end) U];
                Z_v = [Z_v(:,2:end) Z];
            end
            
            % Subplots
            if mod(i,20) == 0
                
                % Incoming EEG in time
                subplot(2,3,1)
                plot(1:win,musict_v)
                axis([1 win -1 1])
                grid on
                xlabel('n')
                ylabel('Amplitude')
                title(['Incoming sequence in time domain, training epoch: '  num2str(ind_C)])
                
                % State X - Mainly Y
                subplot(2,3,2)
                plot(1:win,X_v');
                grid on
                xlabel('n')
                ylabel('Amplitude')
                title('State x')
                
                % State U
                subplot(2,3,3)
                [dmb2,maxU] = max(U_v(:,win));
                plot(U_v(maxU,1:win)');
                grid on
                xlabel('n')
                ylabel('Amplitude')
                title('State u')
                
                % State Z
                subplot(2,3,4)
                stem(0:2,[Z_v(1,win) Z_v(2,win) Z_v(3,win)])
                xlabel('n')
                ylabel('Amplitude')
                title('State z at last epoch of obs window')
                
                % Clusters on third layer
                subplot(2,3,5)
                plot3(Z_v(1,1:win),Z_v(2,1:win),Z_v(3,1:win))
                hold on
                plot3(Z_v(1,win),Z_v(2,win),Z_v(3,win),'--rs');
                hold off
                %title(num2str(sqrt(var(Z_v'))))
                title('3rd layer througout obs window')
                
                pause(0.01)
                
                
            end
        end
    end
end


for i = auxind:length(EEGt)

    if mod(i,round(se*Fs)) == 0 && i > auxind
        disp(['Finishing centers epoch ' num2str(i/round(se*Fs)) ' Variance for z ' num2str(var(Z_v'))])
        mZ = mean(Z_v');
        centers(:,stg(i)) = mean(Z_v');
        subplot(3,4,12)
        if stg(i) <= 2
            plot3(mZ(1),mZ(2),mZ(3),'color','green','Linewidth',1,'Marker','o','MarkerFaceColor','green','MarkerSize',3.75)
            hold on
        elseif stg(i) >= 3 && stg(i) <= 7
            plot3(mZ(1),mZ(2),mZ(3),'color','red','Linewidth',1,'Marker','o','MarkerFaceColor','red','MarkerSize',3.75)
            hold on
        elseif stg(i) >= 8 && stg(i) <= 12
            plot3(mZ(1),mZ(2),mZ(3),'color','blue','Linewidth',1,'Marker','o','MarkerFaceColor','blue','MarkerSize',3.75)
            hold on
        elseif stg(i) >= 13
            plot3(mZ(1),mZ(2),mZ(3),'color','black','Linewidth',1,'Marker','o','MarkerFaceColor','black','MarkerSize',3.75)
            hold on
        end
        title('Clusters per stage')

    end
    
   
    
   
    
    % Subplots
    if mod(i,20) == 0
        
        % Incoming EEG in time
        subplot(3,4,1)
        plot(1:win,musict_v)
        axis([1 win -1 1])
        grid on
        xlabel('n')
        ylabel('Amplitude')
        title(['Incoming EEG, training epoch: ' num2str(ceil(i/(se*Fs)))])
        
        % Output of delta filters
        subplot(3,4,2)
        plot(1:win,filtpp_v(1:2,:)')
        grid on
        xlabel('n')
        ylabel('Amplitude')
        title('Delta filters')
        
        % Output of theta filters
        subplot(3,4,3)
        plot(1:win,filtpp_v(3:7,:)')
        grid on
        xlabel('n')
        ylabel('Amplitude')
        title('Theta filters')
        
        % Output of alpha filters
        subplot(3,4,4)
        plot(1:win,filtpp_v(8:12,:)')
        grid on
        xlabel('n')
        ylabel('Amplitude')
        title('Alpha filters')
        
        % Delta RMS
        subplot(3,4,5)
        plot(1:win,IIR_v(1:2,:)')
        grid on
        xlabel('n')
        ylabel('Amplitude')
        title('Delta MS')
        % for this case
        axis([1 300 0 1])
        
        % Theta RMS
        subplot(3,4,6)
        plot(1:win,IIR_v(3:7,:)')
        grid on
        xlabel('n')
        ylabel('Amplitude')
        title('Theta MS')
        % for this case
        axis([1 300 0 1])
        
        % Alpha RMS
        subplot(3,4,7)
        plot(1:win,IIR_v(8:12,:)')
        grid on
        xlabel('n')
        ylabel('Amplitude')
        title('Alpha MS')
        % for this case
        axis([1 300 0 1])
        
        % State X - Mainly Y
        subplot(3,4,8)
        %[dmb1,maxX] = max(X_v(:,ct));
        plot(1:win,X_v');
        grid on
        xlabel('n')
        ylabel('Amplitude')
        title('State x')
        
        % State U
        subplot(3,4,9)
        [dmb2,maxU] = max(U_v(:,win));
        plot(U_v(maxU,1:win)');
        grid on
        xlabel('n')
        ylabel('Amplitude')
        title('State u')
        
        % -dimensional plot of third layer through n training epochs
%         subplot(3410)
%         plot(Ycorr)
%         grid on
%         title(['Correlation, tau = ' num2str(tau-1)])
%         %             if s == 2
%         %                 plot(Z(1,1:ct),Z(2,1:ct))
%         %                 hold on
%         %                 plot(Z(1,ct),Z(2,ct),'--rs');
%         %                 hold off
%         %             elseif s == 3
%         %                 plot3(Z(1,1:ct),Z(2,1:ct),Z(3,1:ct))
%         %                 hold on
%         %                 plot3(Z(1,ct),Z(2,ct),Z(3,ct),'--rs');
%         %                 hold off
%         %             end
        
        % State Z
        subplot(3,4,10)
        if s == 2
            stem(0:1,[Z_v(1,win) Z_v(2,win)])
            xlabel('n')
            ylabel('Amplitude')
            title('State z at last epoch of obs window')
            %axis([-1 2 0.9*min([Z_v(1,win) Z_v(2,win)]) 1.1*max([Z_v(1,win) Z_v(2,win)])])
            %hold on
        elseif s == 3
            stem(0:2,[Z_v(1,win) Z_v(2,win) Z_v(3,win)])
            xlabel('n')
            ylabel('Amplitude')
            title('State z at last epoch of obs window')
            %axis([-1 3 0.9*min([Z_v(1,win) Z_v(2,win) Z_v(3,win)]) 1.1*max([Z_v(1,win) Z_v(2,win) Z_v(3,win)])])
            %hold on
        end
        
        % Clusters on third layer
        subplot(3,4,11)
        if s == 2
            plot(Z_v(1,1:win),Z_v(2,1:win))
            hold on
            plot(Z_v(1,win),Z_v(2,win),'--rs');
            hold off
            title('3rd layer througout obs window')
            %title(num2str(sqrt(var(Z_v'))))
        elseif s == 3
            plot3(Z_v(1,1:win),Z_v(2,1:win),Z_v(3,1:win))
            hold on
            plot3(Z_v(1,win),Z_v(2,win),Z_v(3,win),'--rs');
            hold off
            %title(num2str(sqrt(var(Z_v'))))
            title('3rd layer througout obs window')
        end
        
        % Clusters per stage
        %subplot(3,4,12)
        
%         if s == 2
%             switch stg(i)
%                 case 1
%                     plot(Z_v(1,1:win),Z_v(2,1:win),'color','green')
%                     %plot(Z_v(1,win),Z_v(2,win),'color','green')
%                     hold on
%                 case 2
%                     plot(Z_v(1,1:win),Z_v(2,1:win),'color','red')
%                     %plot(Z_v(1,win),Z_v(2,win),'color','red')
%                     hold on
%                 case 3
%                     plot(Z_v(1,1:win),Z_v(2,1:win),'color','blue')
%                     %plot(Z_v(1,win),Z_v(2,win),'color','blue')
%                     hold on
%             end
%             title('Clusters per stage')
%         elseif s == 3
%             if stg(i) <= 2
%                 plot3(Z_v(1,win),Z_v(2,win),Z_v(3,win),'color','green','Linewidth',1,'Marker','o','MarkerSize',3.75)
%                 hold on
%             elseif stg(i) >= 3 && stg(i) <= 7
%                 plot3(Z_v(1,win),Z_v(2,win),Z_v(3,win),'color','red','Linewidth',1,'Marker','o','MarkerSize',3.75)
%                 hold on
%             elseif stg(i) >= 8 && stg(i) <= 12
%                 plot3(Z_v(1,win),Z_v(2,win),Z_v(3,win),'color','blue','Linewidth',1,'Marker','o','MarkerSize',3.75)
%                 hold on
%             elseif stg(i) >= 13
%                 plot3(Z_v(1,win),Z_v(2,win),Z_v(3,win),'color','black','Linewidth',1,'Marker','o','MarkerSize',3.75)
%                 hold on
%             end
%             title('Clusters per stage')
%         end
        
      
        pause(0.001)
        
    end
end

end