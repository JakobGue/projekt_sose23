import plotly.graph_objects as go
from nicegui import app, ui


def create() -> None:
    @ui.page('/')
    def dashboard():
        app.add_static_files("/stl", "static")

        with ui.dialog() as dialog, ui.card():
            ui.input(label='Process Name', placeholder='',
                on_change=lambda e: process_name.set_text(f"Process: {e.value}"))
            ui.button('Start', on_click=dialog.close)


        with ui.card() as card:                 
            with ui.row():
                ui.button('Start', on_click=dialog.open)
                ui.button('Stop', on_click=lambda: ui.notify('Stopped'))

                with ui.button('Load Data', on_click=lambda: menu.open()):
                    with ui.menu() as menu:
                        ui.menu_item('Close', on_click=menu.close)
                ui.button('Export', on_click=lambda: ui.notify('Exporting...'))
                process_name = ui.label()

        with ui.row():
            with ui.scene(width=600, height=450) as scene:
                scene.spot_light(distance=100, intensity=0.1).move(-20, 0, 10)
                car = scene.stl("./stl/car.stl").scale(0.3).move(0, 0)
                omega, phi, kappa = 0, 0, 3.14
                car.rotate(omega=omega, phi=phi, kappa=kappa)
                car.color = "grey"

            with ui.column():
                ui.circular_progress(min=0, max=100, value='80', size='8rem')._props['color'] = 'green-400' # type: ignore

                fig = go.Figure(go.Scatter(x=[1, 2, 3, 4], y=[1, 2, 3, 2.5]))
                fig.update_layout(margin=dict(t=0, b=0))
                ui.plotly(fig).classes('h-80')

                

    ui.run(title="Projekt4 POC", dark=False)
