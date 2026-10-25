%
% Description:
%   Establishes UART communication between PC (MATLAB) and MCU.
%   PC sends the start character 's' to the MCU to trigger transmission.
%   MCU then sends 10 rounds of measurement data as formatted strings,
%   each terminated with \r\n (carriage return + newline).
%   MATLAB reads and prints each full line to the Command Window.
%
%   MCU sends lines in the format:
%   "measurement #(i), data1 (x), data2 (x), data3 (x)\r\n"
%
% MCU Baud Rate: 115200 bps (PIOSC 16MHz, IBRD=8, FBRD=44)
%
% Usage:
%   1. Flash the 2DX_2022_Studio_E2 Keil project onto your MCU.
%   2. Update the 'port' variable below to match your COM port.
%      - Windows: "COM3", "COM4", etc.
%      - Mac:     "/dev/cu.usbmodemXXXXXX"
%      Run >> serialportlist    in MATLAB Command Window to find your port.
%   3. Run this script.

%% Configuration 
port     = "COM3";       % <-- CHANGE THIS to your MCU's serial port
baudrate = 115200;       % Must match MCU UART_Init() setting

%% Open the serial port 
device = serialport(port, baudrate);
device.Timeout = 30;     % 10 second read timeout

% Configure line terminator to match MCU's \r\n output
% readline() will read until it sees a newline (LF = "\n")
configureTerminator(device, "CR/LF");

fprintf("Opening: %s\n", port);

%% Flush / reset the buffers
flush(device);

%% Wait for user to press Enter 
input("Press Enter to start communication...");

%% Send start flag 's' to MCU via UART
write(device, 's', "char");

%% Receive 10 lines of measurement data from MCU 
fprintf("\nReceived measurements:\n");
fileID = fopen('tof_radar.xyz', 'w');

for j = 1:8 %for 8 measurement loops 
angles = 0:5.625:354.375; % varies angle from 0-360
disty = 0:10:80; % distance increases by 1 meter each time

for i= 1:64
    line = readline(device);%gets distance from device
    distance = str2double(line);
    distance = distance/1000; %convert to meters

    angle = deg2rad(angles(i));
    
    %% xyz calculations mentioned in report
    x = disty(j)/10;
    y = distance*sin(angle);
    z = distance*cos(angle);

    fprintf("Measurement %d: %.3f m at %d deg corresponding to (%.3f, %.3f, %.3f)\n", i, distance, angles(i), x, y, z);
    fprintf(fileID, "%.3f %.3f %.3f\n", x, y, z);
end
end
fclose(fileID);

%% Close the serial port 
fprintf("Closing: %s\n", port);
clear device;
%load data to file
data = load('tof_radar.xyz');
x = data(:,1);
y = data(:,2);
z = data(:,3);

figure;
hold on;

for j = 1:3
    idx = ((j-1)*64 + 1) : (j*64);  % indices for this height slice
    xj = x(idx);
    yj = y(idx);
    zj = z(idx);
    
    % Plot this ring and close it
    plot3([xj; xj(1)], [yj; yj(1)], [zj; zj(1)], ...
          '-o', 'LineWidth', 2, 'MarkerSize', 5, 'MarkerFaceColor', 'r');
end

% Connect corresponding angles across heights (vertical lines)
for i = 1:64
    xv = x(i:64:end);
    yv = y(i:64:end);
    zv = z(i:64:end);
    plot3(xv, yv, zv, 'b-', 'LineWidth', 1);
end
%make final labels for plot
xlabel('X (m)'); ylabel('Y (m)'); zlabel('Z (m)');
title('ToF 3D Scan');
grid on;
axis equal;