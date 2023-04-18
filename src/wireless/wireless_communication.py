import serial

def read_serial(prev_o, prev_p, prev_k):
    try:
        ser = serial.Serial('COM10', 9600)
    except serial.serialutil.SerialException:
        return prev_o, prev_p, prev_k
    try:
        line = ser.readline().decode('utf-8').rstrip()
    except UnicodeDecodeError:
        return prev_o, prev_p, prev_k

    values = line.split(',')
    if len(values) == 3:
        try:
            o, p, k = float(values[0].split(': ')[1]), float(values[1].split(': ')[1]), float(values[2].split(': ')[1])
            if (o, p, k) != (prev_o, prev_p, prev_k):
                return float(values[0].split(': ')[1]), float(values[1].split(': ')[1]), float(values[2].split(': ')[1])
            else:
                return prev_o, prev_p, prev_k
        except IndexError:
            return prev_o, prev_p, prev_k
    else:
        return prev_o, prev_p, prev_k