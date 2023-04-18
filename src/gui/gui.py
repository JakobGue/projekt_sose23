import plotly.graph_objects as go
from nicegui import app, ui
from wireless.wireless_communication import read_serial
from datetime import datetime
import pandas as pd

omega, phi, kappa = 0, 0, 0
df = pd.DataFrame(columns=['time', 'omega', 'phi', 'kappa'])
def create() -> None:
    
    @ui.page('/')
    def dashboard():
        app.add_static_files("/stl", "static")

        def start():
            global df
            df = df.iloc[0:0]
            fig.update_figure(go.Figure([go.Scatter(x=[], y=[]) for col in ['omega', 'phi', 'kappa']]))
            updater.active = True
            dialog.close()

        def stop():
            updater.active = False

        with ui.dialog() as dialog, ui.card():
            ui.input(label='Process Name', placeholder='',
                on_change=lambda e: process_name.set_text(f"Process: {e.value}"))
            ui.button('Start', on_click=start)


        with ui.card() as card:                 
            with ui.row():
                ui.button('Start', on_click=dialog.open)
                ui.button('Stop', on_click=stop)

                with ui.button('Load Data', on_click=lambda: menu.open()):
                    with ui.menu() as menu:
                        ui.menu_item('Close', on_click=menu.close)
                ui.button('Export', on_click=lambda: ui.notify('Exporting...'))
                process_name = ui.label()

        with ui.row():
            with ui.scene(width=600, height=450) as scene:
                scene.spot_light(distance=100, intensity=0.1).move(-20, 0, 10)
                car = scene.stl("./stl/car.stl").scale(0.3).move(0, 0)
                
                car.rotate(omega=omega, phi=phi, kappa=3.14)
                car.color = "grey"

                

            with ui.column():
                ui.circular_progress(min=0, max=100, value='80', size='8rem')._props['color'] = 'green-400' # type: ignore
                fig = ui.plotly(go.Figure([go.Scatter(x=df.time, y=df[col]) for col in ['omega', 'phi', 'kappa']]))


            def update():
                global omega, phi, kappa, df
                prev_omega, prev_phi, prev_kappa = omega, phi, kappa
                omega, phi, kappa = read_serial(omega, phi, kappa)
                if (omega, phi, kappa) == (prev_omega, prev_phi, prev_kappa):
                    car.rotate(omega=omega, phi=phi, kappa=3.14)
                    now = datetime.now()
                df.loc[len(df)] = [now, omega, phi, kappa]
                fig.update_figure(go.Figure([go.Scatter(x=df.time, y=df[col]) for col in ['omega', 'phi', 'kappa']]))

            updater = ui.timer(0.1, update, active=False)

        

        

                

    ui.run(title="Projekt4 POC", dark=False)
