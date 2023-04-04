from nicegui import app, ui
from datetime import datetime
import serial

app.add_static_files("/stl", "static")


def get_random_rotation(omega, phi, kappa):
    # function to smoothly rotate the car near to previos rotation
    # omega, phi, kappa = previous rotation
    import random

    # get random rotation between -0.2 and 0.2
    omega = random.uniform(-0.1, 0.1)
    phi = random.uniform(-0.1, 0.1)
    kappa = random.uniform(-0.1, 0.1)

    return omega, phi, kappa

def read_serial(prev_o, prev_p, prev_k):
    try:
        ser = serial.Serial('COM9', 9600)
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

line_plot = ui.line_plot(n=3, limit=20, figsize=(5, 3), update_every=5).with_legend(
    ["x", "y", "z"], loc="upper center", ncol=2
)

with ui.scene(width=800, height=500) as scene:
    scene.spot_light(distance=100, intensity=0.1).move(-20, 0, 10)
    car = scene.stl("./stl/car.stl").scale(0.3).move(0, 0)
    omega, phi, kappa = 0, 0, 3.14
    car.rotate(omega=omega, phi=phi, kappa=kappa)
    car.color = "silver"

    def move():
        global omega, phi, kappa
        prev_omega, prev_phi, prev_kappa = omega, phi, kappa
        omega, phi, kappa = read_serial(omega, phi, kappa)
        if (omega, phi, kappa) == (prev_omega, prev_phi, prev_kappa):
            car.rotate(omega=omega, phi=phi, kappa=3.14)

            now = datetime.now()
            line_plot.push([now], [[omega], [phi], [kappa]])

    ui.timer(0.1, move)



ui.run(title="Projekt4 POC", dark=True)
