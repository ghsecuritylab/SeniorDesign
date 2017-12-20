function observations=prepFreqObservations(tone,fft_size,window_size)
%tone has to be a one dimensional column vector
lengthTone = length(tone);
windowCount = ceil((lengthTone-window_size)/(window_size/2));
append = window_size + windowCount*(window_size/2) - lengthTone;
tone = [tone ; zeros(append,1)];
in=1;
col=1;
tone=tone./abs(max(tone));
window=hann(window_size);
observations=zeros(fft_size/2,floor(2*length(tone)/window_size)-1);
while in+window_size<length(tone)
    data=tone(in:in+window_size-1);
    if abs(max(data))>0.1
        data=data./abs(max(data));
    end
    datawindow=data.*window;
    fullfft=abs(fft(datawindow,fft_size));
    observations(:,col)=fullfft(1:fft_size/2);
%     image(observations);
    in=in+window_size*5/10;
    col=col+1;
end
% observations = 1.*observations./max(max(observations));
end