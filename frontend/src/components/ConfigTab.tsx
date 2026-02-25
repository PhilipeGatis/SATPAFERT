import { useState } from 'react';
import { api } from '../App';

export default function ConfigTab() {
    const [ssid, setSsid] = useState('');
    const [pass, setPass] = useState('');
    const [networks, setNetworks] = useState<{ ssid: string; rssi: number }[]>([]);
    const [scanning, setScanning] = useState(false);

    const handleScan = async () => {
        setScanning(true);
        const poll = async () => {
            try {
                const r = await fetch('/api/wifi/scan');
                if (r.status === 202) {
                    setTimeout(poll, 1500);
                    return;
                }
                if (r.ok) {
                    const d = await r.json();
                    setNetworks(d.networks);
                }
            } catch (err) {
                alert('Erro ao buscar redes.');
            }
            setScanning(false);
        };
        poll();
    };

    const handleSave = async (e: React.FormEvent) => {
        e.preventDefault();
        const fd = new URLSearchParams();
        fd.append('ssid', ssid);
        fd.append('pass', pass);
        try {
            const r = await fetch('/api/wifi', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: fd.toString(),
            });
            if (r.ok) alert('WiFi configurado! O sistema reiniciará para conectar.');
            else alert('Erro ao salvar WiFi.');
        } catch {
            alert('Erro de comunicação.');
        }
    };

    return (
        <div className="flex flex-col gap-4">
            <div className="rounded-2xl bg-card p-5 shadow-md">
                <h2 className="mb-4 text-base font-medium tracking-wide text-text/90 uppercase">Configuração de Rede</h2>

                <form onSubmit={handleSave} className="flex flex-col gap-4">
                    <div className="flex gap-2">
                        <select
                            value={ssid}
                            onChange={(e) => setSsid(e.target.value)}
                            className="w-full flex-1 rounded-t-md border-b-2 border-muted bg-white/5 px-3 py-2 text-sm text-text outline-none transition-colors focus:border-accent"
                            required
                        >
                            <option value="" disabled className="bg-card">
                                {scanning ? 'Buscando...' : networks.length ? 'Selecione a rede' : 'Nenhuma rede (clique buscar)'}
                            </option>
                            {networks.map((n, i) => (
                                <option key={i} value={n.ssid} className="bg-card">
                                    {n.ssid} ({n.rssi}dBm)
                                </option>
                            ))}
                        </select>
                        <button
                            type="button"
                            onClick={handleScan}
                            disabled={scanning}
                            className="rounded-full bg-white/5 px-4 font-bold text-muted transition hover:bg-white/10 active:scale-95 disabled:opacity-50"
                        >
                            {scanning ? '...' : '↻'}
                        </button>
                    </div>

                    <div className="flex flex-col gap-1">
                        <label className="text-xs font-bold text-muted uppercase tracking-wider">Senha da Rede</label>
                        <input
                            type="password"
                            placeholder="*************"
                            value={pass}
                            onChange={(e) => setPass(e.target.value)}
                            className="w-full rounded-t-md border-b-2 border-muted bg-white/5 px-3 py-2 text-sm text-text outline-none transition-colors focus:border-accent"
                        />
                    </div>

                    <button
                        type="submit"
                        className="mt-4 rounded-full bg-accent2 px-6 py-2.5 text-sm font-bold uppercase tracking-wider text-black shadow-md transition-all hover:bg-teal-300 active:scale-95"
                    >
                        Salvar e Reiniciar
                    </button>
                </form>
            </div>

            <div className="rounded-2xl border border-danger/50 bg-danger/10 p-5 shadow-md">
                <h2 className="mb-2 text-base font-medium tracking-wide text-danger uppercase">Ações de Emergência</h2>
                <button
                    onClick={() => api('POST', '/api/maintenance')}
                    className="mt-2 w-full rounded-full bg-warn/20 py-3 text-sm font-bold uppercase tracking-wider text-warn transition hover:bg-warn/30 active:scale-95"
                >
                    Pausar TPA / Ligar Manutenção
                </button>
            </div>
        </div>
    );
}
