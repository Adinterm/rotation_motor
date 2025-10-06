import ttkbootstrap as ttk
from ttkbootstrap.constants import *
from tkinter import scrolledtext, messagebox
import serial
import serial.tools.list_ports
import threading


class SerialGUI:
    def __init__(self, master):
        self.master = master
        self.master.title("USB Serial Communication")
        self.serial_conn = None
        self.running = False

        # Make layout expandable
        for i in range(6):  # Columns
            master.columnconfigure(i, weight=1)
        master.rowconfigure(2, weight=1)  # Output box row is expandable

        # === Row 0: Port, Baud, Buttons ===
        ttk.Label(master, text="Port:").grid(row=0, column=0, padx=5, pady=5, sticky='e')
        self.port_combobox = ttk.Combobox(master, values=self.get_serial_ports(), width=15)
        self.port_combobox.grid(row=0, column=1, padx=5, sticky='we')

        self.refresh_button = ttk.Button(master, text="Refresh", command=self.refresh_ports)
        self.refresh_button.grid(row=0, column=2, padx=5, sticky='we')

        ttk.Label(master, text="Baud rate:").grid(row=0, column=3, padx=5, sticky='e')
        self.baud_combobox = ttk.Combobox(
            master, values=["9600", "19200", "38400", "57600", "115200"], width=10
        )
        self.baud_combobox.set("9600")
        self.baud_combobox.grid(row=0, column=4, padx=5, sticky='we')

        self.connect_button = ttk.Button(master, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=0, column=5, padx=5, sticky='we')

        # === Row 1: Output label ===
        ttk.Label(master, text="Serial Monitor Output:").grid(
            row=1, column=0, columnspan=6, padx=10, sticky='w'
        )

        # === Row 2: Output Text Area (scrollable) ===
        self.output_text = scrolledtext.ScrolledText(
            master, wrap='word', height=20, bg='#1e1e1e', fg='white', insertbackground='white'
        )
        self.output_text.grid(row=2, column=0, columnspan=6, padx=10, pady=(0, 10), sticky='nsew')
        self.output_text.configure(state='disabled')

        # === Row 3: Input Entry and Send Button ===
        self.send_entry = ttk.Entry(master)
        self.send_entry.grid(row=3, column=0, columnspan=5, padx=10, pady=5, sticky='we')
        self.send_entry.bind('<Return>', lambda e: self.send_data())

        self.send_button = ttk.Button(master, text="Send", command=self.send_data)
        self.send_button.grid(row=3, column=5, padx=5, pady=5, sticky='we')

        # === Row 4: Clear Button ===
        self.clear_button = ttk.Button(master, text="Clear Output", command=self.clear_output)
        self.clear_button.grid(row=4, column=0, columnspan=6, pady=5, padx=10, sticky='we')

    def get_serial_ports(self):
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def refresh_ports(self):
        self.port_combobox['values'] = self.get_serial_ports()

    def toggle_connection(self):
        if self.serial_conn and self.serial_conn.is_open:
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        port = self.port_combobox.get()
        baud = self.baud_combobox.get()
        if not port or not baud:
            messagebox.showerror("Error", "Select a port and baud rate.")
            return

        try:
            self.serial_conn = serial.Serial(port, int(baud), timeout=1)
            self.running = True
            self.connect_button.config(text="Disconnect")
            threading.Thread(target=self.read_from_serial, daemon=True).start()
            self.log_message(f"[INFO] Connected to {port} at {baud} baud.")
        except serial.SerialException as e:
            messagebox.showerror("Connection Error", str(e))

    def disconnect(self):
        self.running = False
        if self.serial_conn:
            self.serial_conn.close()
        self.connect_button.config(text="Connect")
        self.log_message("[INFO] Disconnected.")

    def read_from_serial(self):
        while self.running:
            try:
                if self.serial_conn.in_waiting > 0:
                    data = self.serial_conn.readline().decode('utf-8', errors='ignore').strip()
                    if data:
                        self.log_message(f"[RX] {data}")
            except serial.SerialException:
                self.log_message("[ERROR] Serial read error.")
                self.disconnect()
                break

    def send_data(self):
        message = self.send_entry.get().strip()
        if self.serial_conn and self.serial_conn.is_open and message:
            try:
                self.serial_conn.write((message + '\n').encode())
                self.log_message(f"[TX] {message}")
                self.send_entry.delete(0, ttk.END)
            except serial.SerialException:
                self.log_message("[ERROR] Failed to send.")
                self.disconnect()
        elif not self.serial_conn or not self.serial_conn.is_open:
            messagebox.showwarning("Not connected", "Connect to a serial port first.")

    def log_message(self, message):
        self.output_text.configure(state='normal')
        self.output_text.insert(ttk.END, message + '\n')
        self.output_text.configure(state='disabled')
        self.output_text.see(ttk.END)

    def clear_output(self):
        self.output_text.configure(state='normal')
        self.output_text.delete(1.0, ttk.END)
        self.output_text.configure(state='disabled')


# === Center window ===
def center_window(root, width=800, height=600):
    root.update_idletasks()
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()
    x = (screen_width // 2) - (width // 2)
    y = (screen_height // 2) - (height // 2)
    root.geometry(f"{width}x{height}+{x}+{y}")


# === Main ===
def main():
    root = ttk.Window(themename="darkly")
    center_window(root, 800, 600)
    app = SerialGUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()
