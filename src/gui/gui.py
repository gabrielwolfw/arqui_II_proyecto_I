#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Interfaz gráfica para ejecutar `make run` del proyecto C++
y mostrar en tiempo real stdout y stderr.
"""
import os
import shlex
import signal
import subprocess
import threading
import queue
import sys
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from tkinter.scrolledtext import ScrolledText

class ProcessRunnerGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Salida del proyecto")
        self.geometry("1000x650")

        # Determina el directorio donde se ejecutará el comando.
        
        this_dir = os.path.dirname(os.path.abspath(__file__))
        default_cwd = os.path.abspath(os.path.join(this_dir, ".."))  # -> src/
        self.repo_root = default_cwd

        self._make_widgets()
        self.process = None
        self.stdout_thread = None
        self.stderr_thread = None
        self.queue = queue.Queue()
        self.after(100, self._poll_queue)

    def _make_widgets(self):
        top_frame = ttk.Frame(self)
        top_frame.pack(side="top", fill="x", padx=8, pady=8)

        ttk.Label(top_frame, text="Comando:").pack(side="left")
        self.cmd_var = tk.StringVar()
        # Por defecto usar make run
        self.cmd_var.set("make run")
        self.cmd_entry = ttk.Entry(top_frame, textvariable=self.cmd_var, width=60)
        self.cmd_entry.pack(side="left", padx=(6, 6))

        ttk.Label(top_frame, text="  (cwd:)").pack(side="left")
        self.cwd_lbl = ttk.Label(top_frame, text=self.repo_root, anchor="w")
        self.cwd_lbl.pack(side="left", padx=(4,0), fill="x", expand=True)

        change_btn = ttk.Button(top_frame, text="Cambiar cwd", command=self.change_cwd)
        change_btn.pack(side="right", padx=4)

        btn_frame = ttk.Frame(self)
        btn_frame.pack(side="top", fill="x", padx=8, pady=(0,8))

        self.start_btn = ttk.Button(btn_frame, text="Iniciar", command=self.start_process)
        self.start_btn.pack(side="left", padx=4)

        self.stop_btn = ttk.Button(btn_frame, text="Detener", command=self.stop_process, state="disabled")
        self.stop_btn.pack(side="left", padx=4)

        self.clear_btn = ttk.Button(btn_frame, text="Borrar", command=self.clear_output)
        self.clear_btn.pack(side="left", padx=4)

        self.save_btn = ttk.Button(btn_frame, text="Guardar log", command=self.save_log)
        self.save_btn.pack(side="left", padx=4)

        # Área de texto con scroll (solo visual)
        self.text = ScrolledText(self, wrap="none", state="disabled", font=("Consolas", 11))
        self.text.pack(fill="both", expand=True, padx=8, pady=(0,8))

        # Config tags
        self.text.tag_configure("stdout", foreground="black")
        self.text.tag_configure("stderr", foreground="red")
        self.text.tag_configure("meta", foreground="blue", font=("Consolas", 9, "italic"))

    def change_cwd(self):
        d = filedialog.askdirectory(initialdir=self.repo_root, title="Selecciona directorio de trabajo (cwd)")
        if d:
            self.repo_root = os.path.abspath(d)
            self.cwd_lbl.config(text=self.repo_root)
            self.append_meta(f"cwd cambiado a: {self.repo_root}\n")

    def start_process(self):
        if self.process is not None:
            messagebox.showinfo("Ya ejecutándose", "El proceso ya está en ejecución.")
            return

        cmdline = self.cmd_var.get().strip()
        if not cmdline:
            messagebox.showerror("Comando vacío", "Especifica el comando a ejecutar (por defecto: make run).")
            return

        # Convertir a lista de argumentos (evitar shell por defecto)
        try:
            args = shlex.split(cmdline)
            use_shell = False
        except Exception:
            args = cmdline
            use_shell = True

        self.append_meta(f"Iniciando: {cmdline}\n(working dir: {self.repo_root})\n")
        try:
            # Crear grupo de procesos para poder terminar procesos hijos también (Unix)
            if os.name == "nt":
                creationflags = subprocess.CREATE_NEW_PROCESS_GROUP
                preexec_fn = None
            else:
                creationflags = 0
                preexec_fn = os.setsid

            self.process = subprocess.Popen(
                args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                stdin=subprocess.PIPE,
                bufsize=1,
                universal_newlines=True,
                cwd=self.repo_root,
                shell=use_shell,
                preexec_fn=preexec_fn,
                creationflags=creationflags
            )
        except Exception as e:
            self.append_meta(f"Error al iniciar el proceso: {e}\n")
            self.process = None
            return

        self.start_btn.config(state="disabled")
        self.stop_btn.config(state="normal")

        # Hilos para leer stdout y stderr
        self.stdout_thread = threading.Thread(target=self._reader_thread, args=(self.process.stdout, "stdout"), daemon=True)
        self.stderr_thread = threading.Thread(target=self._reader_thread, args=(self.process.stderr, "stderr"), daemon=True)
        self.stdout_thread.start()
        self.stderr_thread.start()

        # hilo que vigila fin de proceso
        threading.Thread(target=self._wait_process, daemon=True).start()

    def stop_process(self):
        if not self.process:
            return
        self.append_meta("Terminando proceso...\n")
        try:
            if os.name == "nt":
                try:
                    self.process.send_signal(signal.CTRL_BREAK_EVENT)
                except Exception:
                    self.process.terminate()
            else:
                # Kill todos los procesos
                try:
                    os.killpg(os.getpgid(self.process.pid), signal.SIGTERM)
                except Exception:
                    os.kill(self.process.pid, signal.SIGTERM)
        except Exception as e:
            self.append_meta(f"Error al terminar proceso: {e}\n")

    def _wait_process(self):
        if not self.process:
            return
        ret = None
        try:
            ret = self.process.wait()
        except Exception as e:
            self.queue.put(("meta", f"Error esperando al proceso: {e}\n"))
        finally:
            self.queue.put(("meta", f"Proceso finalizado con código de salida: {ret}\n"))
            # marca fin de proceso para UI
            self.queue.put(("__proc_end__", None))

    def _reader_thread(self, pipe, tag):
        try:
            for line in iter(pipe.readline, ""):
                if not line:
                    break
                self.queue.put((tag, line))
        except Exception as e:
            self.queue.put(("meta", f"Error leyendo {tag}: {e}\n"))
        finally:
            try:
                pipe.close()
            except Exception:
                pass

    def _poll_queue(self):
        try:
            while True:
                item = self.queue.get_nowait()
                if not item:
                    continue
                tag, data = item
                if tag == "__proc_end__":
                    self.process = None
                    self.start_btn.config(state="normal")
                    self.stop_btn.config(state="disabled")
                    continue
                self._append_to_text(data, tag)
        except queue.Empty:
            pass
        self.after(100, self._poll_queue)

    def _append_to_text(self, text, tag):
        def _do():
            self.text.configure(state="normal")
            self.text.insert("end", text, (tag,))
            self.text.see("end")
            self.text.configure(state="disabled")
        self.after(0, _do)

    def append_meta(self, text):
        self._append_to_text(text, "meta")

    def clear_output(self):
        self.text.configure(state="normal")
        self.text.delete("1.0", "end")
        self.text.configure(state="disabled")

    def save_log(self):
        initial = "output.log"
        f = filedialog.asksaveasfilename(defaultextension=".log", initialfile=initial,
                                         filetypes=[("Text files", "*.txt *.log"), ("All files","*.*")])
        if not f:
            return
        try:
            content = self.text.get("1.0", "end")
            with open(f, "w", encoding="utf-8") as fh:
                fh.write(content)
            messagebox.showinfo("Guardado", f"Log guardado en:\n{f}")
        except Exception as e:
            messagebox.showerror("Error al guardar", str(e))

if __name__ == "__main__":
    app = ProcessRunnerGUI()
    app.mainloop()