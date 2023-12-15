from flask import Flask, render_template_string, request
import telnetlib

app = Flask(__name__)

telnet_host = '192.168.137.60'
telnet_port = 2121

@app.route('/')
def index():
    with open('index.html', 'r') as file:
        html_content = file.read()
    return html_content

@app.route('/send_command', methods=['POST'])
def send_command():
    command = request.form.get('command')
    try:
        tn = telnetlib.Telnet(telnet_host, telnet_port)

        if command == 'run':
            tn.write(b'run\r\n')
        elif command.startswith('set cycle'):
            cycle_time = command.split(' ')[-1]
            tn.write(f'set cycle {cycle_time}\r\n'.encode('utf-8'))
        elif command.startswith('set'):
            parameter, value = command.split(' ')[1], command.split(' ')[-1]
            tn.write(f'{command}\r\n'.encode('utf-8'))

        tn.close()
        print(f"Sent command: {command} to the robot via Telnet")
    except Exception as e:
        print(f"Error in Telnet connection: {e}")

    return "Command sent successfully"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
