import tkinter as tk
from tkinter import font

class RealisticLCDSimulator:
    def __init__(self, root):
        self.root = root
        self.root.title("Realistic 16x2 LCD Simulator")
        self.root.geometry("600x400")
        self.root.configure(bg="#333333") # Dark grey background

        # --- PCB BOARD (The Green Base) ---
        # This frame represents the green circuit board.
        self.pcb_frame = tk.Frame(root, bg="#007a33", bd=0, padx=20, pady=20)
        self.pcb_frame.pack(pady=20)

        # --- WIRES AND PINS (Left Side) ---
        # A canvas to draw the pins and wires.
        self.wires_canvas = tk.Canvas(self.pcb_frame, width=60, height=100, bg="#007a33", highlightthickness=0)
        self.wires_canvas.pack(side="left", fill="y")

        # Pin labels and colors
        pins = [("GND", "black"), ("VCC", "red"), ("SDA", "#00ff00"), ("SCL", "#00ff00")]
        y_start = 20
        for i, (label, color) in enumerate(pins):
            y = y_start + i * 20
            # Draw the pin (a small silver rectangle)
            self.wires_canvas.create_rectangle(45, y-4, 55, y+4, fill="#c0c0c0", outline="#808080")
            # Draw the wire extending to the left
            self.wires_canvas.create_line(0, y, 45, y, fill=color, width=3)
            # Draw the pin label text
            self.wires_canvas.create_text(35, y, text=label, fill="white", font=("Arial", 8), anchor="e")
        
        # Draw the pin number '1'
        self.wires_canvas.create_text(40, y_start - 15, text="1", fill="white", font=("Arial", 8))

        # --- LCD BEZEL (The Black Frame) ---
        # The black frame around the screen area.
        self.bezel_frame = tk.Frame(self.pcb_frame, bg="black", bd=0, padx=15, pady=15)
        self.bezel_frame.pack(side="left")

        # --- LCD SCREEN GRID ---
        # A frame to hold the 16x2 grid of character cells.
        self.grid_frame = tk.Frame(self.bezel_frame, bg="#1a3300") # Dark green background between cells
        self.grid_frame.pack()

        # Font for the characters (monospaced is best)
        self.lcd_font = font.Font(family="Courier New", size=18, weight="bold")
        
        # Colors for the lit and unlit states
        self.cell_bg = "#284b00" # Dark green for unlit pixels
        self.text_color = "#76c900" # Bright lime green for lit pixels

        # A 2D list to store the label widgets for each character cell
        self.cells = [[None for _ in range(16)] for _ in range(2)]

        # Create the 16x2 grid of labels
        for row in range(2):
            for col in range(16):
                # Each cell is a label. We use border and highlightthickness to create the grid lines.
                cell = tk.Label(
                    self.grid_frame,
                    text=" ", # Start empty
                    bg=self.cell_bg,
                    fg=self.text_color,
                    font=self.lcd_font,
                    width=1, # Width in characters
                    height=1, # Height in characters
                    bd=1,     # Border width
                    relief="solid", # Solid border style
                    highlightbackground="#1a3300", # Color of the border (grid line)
                    highlightcolor="#1a3300",
                    highlightthickness=1 # Thickness of the highlight border
                )
                cell.grid(row=row, column=col, padx=0, pady=0)
                self.cells[row][col] = cell

        # --- CONTROLS AREA ---
        self.control_frame = tk.Frame(root, bg="#333333")
        self.control_frame.pack(pady=10)

        # Labels and Entry boxes with updated styling
        label_style = {"bg": "#333333", "fg": "#cccccc", "font": ("Arial", 10)}
        entry_style = {"bg": "#555555", "fg": "white", "insertbackground": "white", "font": ("Courier New", 12)}

        tk.Label(self.control_frame, text="Line 1:", **label_style).grid(row=0, column=0, padx=5, sticky="e")
        self.entry_line1 = tk.Entry(self.control_frame, width=20, **entry_style)
        self.entry_line1.grid(row=0, column=1, padx=5, pady=5)
        
        tk.Label(self.control_frame, text="Line 2:", **label_style).grid(row=1, column=0, padx=5, sticky="e")
        self.entry_line2 = tk.Entry(self.control_frame, width=20, **entry_style)
        self.entry_line2.grid(row=1, column=1, padx=5, pady=5)

        # Button
        self.btn_update = tk.Button(self.control_frame, text="UPDATE DISPLAY", command=self.update_lcd, bg="#444444", fg="white", activebackground="#555555", activeforeground="white", font=("Arial", 10, "bold"), bd=1, relief="raised")
        self.btn_update.grid(row=2, column=0, columnspan=2, pady=15, sticky="ew")

        # Bind "Enter" key to update function
        self.root.bind('<Return>', lambda event: self.update_lcd())
        
        # Set initial text
        self.entry_line1.insert(0, "Hello, World!")
        self.entry_line2.insert(0, "LCD Simulator")
        self.update_lcd()

    def update_lcd(self):
        # Get input
        t1 = self.entry_line1.get()
        t2 = self.entry_line2.get()

        # Pad/truncate to exactly 16 characters
        line1_chars = list(t1[:16].ljust(16))
        line2_chars = list(t2[:16].ljust(16))

        # Update each cell in the grid
        for col in range(16):
            self.cells[0][col].config(text=line1_chars[col])
            self.cells[1][col].config(text=line2_chars[col])

if __name__ == "__main__":
    root = tk.Tk()
    app = RealisticLCDSimulator(root)
    root.mainloop()
