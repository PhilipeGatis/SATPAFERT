import { useState, useEffect } from 'react';
import { api, type AQStatus } from '../App';

export default function ConfigTab({ status }: { status: AQStatus | null }) {
    const [ssid, setSsid] = useState('');
    const [pass, setPass] = useState('');
    const [networks, setNetworks] = useState<{ ssid: string; rssi: number }[]>([]);
    const [scanning, setScanning] = useState(false);
    const [aqH, setAqH] = useState('');
    const [aqL, setAqL] = useState('');
    const [aqW, setAqW] = useState('');
    const [primeRatio, setPrimeRatio] = useState('');
    const [reservoirVolume, setReservoirVolume] = useState('');
    const [aqMargin, setAqMargin] = useState('');

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

    useEffect(() => {
        if (status) {
            if (!aqH && status.aqHeight) setAqH(status.aqHeight.toString());
            if (!aqL && status.aqLength) setAqL(status.aqLength.toString());
            if (!aqW && status.aqWidth) setAqW(status.aqWidth.toString());
            if (!aqMargin && status.aqMarginCm !== undefined) setAqMargin(status.aqMarginCm.toString());
            if (!primeRatio && status.primeRatio !== undefined) setPrimeRatio(status.primeRatio.toString());
            if (!reservoirVolume && status.reservoirVolume !== undefined) setReservoirVolume(status.reservoirVolume.toString());
        }
    }, [status]);

    const h = parseInt(aqH) || 0;
    const l = parseInt(aqL) || 0;
    const w = parseInt(aqW) || 0;
    const computedVolume = (h - (parseInt(aqMargin) || 0)) * l * w / 1000;
    const computedLPerCm = l * w / 1000;
    const computedPrime = (parseFloat(reservoirVolume) || 0) * (parseFloat(primeRatio) || 0);

    return (
        <div className="flex flex-col gap-4">
            {/* AQUARIUM CONFIG CARD */}
            <div className="rounded-2xl bg-card p-5 shadow-md">
                <h2 className="mb-4 text-base font-medium tracking-wide text-text/90 uppercase">Configuração do Aquário</h2>
                <div className="flex flex-col gap-4">
                    <div className="flex flex-col gap-1">
                        <label className="text-xs font-bold text-muted uppercase tracking-wider">Dimensões do Aquário (cm)</label>
                        <div className="flex gap-2">
                            <div className="flex-1">
                                <input type="number" min="0" max="999" step="1" placeholder="Altura"
                                    className="w-full rounded-md border-b-2 border-muted bg-white/5 px-3 py-2 text-sm text-text outline-none transition-colors focus:border-accent"
                                    value={aqH} onChange={e => setAqH(e.target.value)} />
                                <span className="text-[9px] text-muted">Altura (A)</span>
                            </div>
                            <span className="self-center text-muted text-lg">×</span>
                            <div className="flex-1">
                                <input type="number" min="0" max="999" step="1" placeholder="Compr."
                                    className="w-full rounded-md border-b-2 border-muted bg-white/5 px-3 py-2 text-sm text-text outline-none transition-colors focus:border-accent"
                                    value={aqL} onChange={e => setAqL(e.target.value)} />
                                <span className="text-[9px] text-muted">Comprimento (C)</span>
                            </div>
                            <span className="self-center text-muted text-lg">×</span>
                            <div className="flex-1">
                                <input type="number" min="0" max="999" step="1" placeholder="Largura"
                                    className="w-full rounded-md border-b-2 border-muted bg-white/5 px-3 py-2 text-sm text-text outline-none transition-colors focus:border-accent"
                                    value={aqW} onChange={e => setAqW(e.target.value)} />
                                <span className="text-[9px] text-muted">Largura (L)</span>
                            </div>
                        </div>
                    </div>
                    <div className="flex flex-col gap-1">
                        <label className="text-xs font-bold text-muted uppercase tracking-wider">Margem da borda (cm)</label>
                        <input type="number" min="0" max="99" step="1" placeholder="Ex: 3"
                            className="w-full rounded-md border-b-2 border-muted bg-white/5 px-3 py-2 text-sm text-text outline-none transition-colors focus:border-accent"
                            value={aqMargin} onChange={e => setAqMargin(e.target.value)} />
                        <span className="text-[10px] text-muted italic">Distância da borda do aquário até a superfície da água quando cheio</span>
                    </div>
                    <div className="rounded-lg bg-accent/10 px-4 py-3 flex flex-col gap-1">
                        <div>
                            <span className="text-xs text-muted">Volume calculado: </span>
                            <strong className="text-accent text-sm">{computedVolume > 0 ? `${computedVolume.toFixed(1)} L` : '--'}</strong>
                        </div>
                        <div>
                            <span className="text-xs text-muted">Litros por cm: </span>
                            <strong className="text-accent2 text-sm">{computedLPerCm > 0 ? `${computedLPerCm.toFixed(2)} L/cm` : '--'}</strong>
                        </div>
                    </div>
                    <div className="flex flex-col gap-1">
                        <label className="text-xs font-bold text-muted uppercase tracking-wider">Proporção de Prime (mL por Litro)</label>
                        <input
                            type="number" min="0" max="10" step="0.001" placeholder="Ex: 0.05"
                            className="w-full rounded-md border-b-2 border-muted bg-white/5 px-3 py-2 text-sm text-text outline-none transition-colors focus:border-accent"
                            value={primeRatio} onChange={e => setPrimeRatio(e.target.value)}
                        />
                        <span className="text-[10px] text-muted italic mt-1">Conforme recomendação do fabricante (Ex: Seachem Prime = 0.05 mL/L)</span>
                    </div>
                    <div className="flex flex-col gap-1">
                        <label className="text-xs font-bold text-muted uppercase tracking-wider">Volume do Reservatório de Tratamento (Litros)</label>
                        <input
                            type="number" min="0" max="9999" step="1" placeholder="Ex: 20"
                            className="w-full rounded-md border-b-2 border-muted bg-white/5 px-3 py-2 text-sm text-text outline-none transition-colors focus:border-accent"
                            value={reservoirVolume} onChange={e => setReservoirVolume(e.target.value)}
                        />
                        <span className="text-[10px] text-muted italic mt-1">Água tratada com Prime antes de repor no aquário</span>
                    </div>
                    <div className="rounded-lg bg-accent/10 px-4 py-3">
                        <span className="text-xs text-muted">Dose de Prime calculada (reservatório): </span>
                        <strong className="text-accent text-sm">{computedPrime > 0 ? `${computedPrime.toFixed(1)} mL` : 'Configure volume do reservatório e proporção'}</strong>
                    </div>
                    <button
                        onClick={() => api('POST', '/api/config/aquarium', {
                            aqHeight: parseInt(aqH) || 0,
                            aqLength: parseInt(aqL) || 0,
                            aqWidth: parseInt(aqW) || 0,
                            aqMarginCm: parseInt(aqMargin) || 0,
                            primeRatio: parseFloat(primeRatio) || 0,
                            reservoirVolume: parseInt(reservoirVolume) || 0
                        })}
                        className="mt-1 rounded-full bg-accent2 px-6 py-2.5 text-sm font-bold uppercase tracking-wider text-black shadow-md transition-all hover:bg-teal-300 active:scale-95"
                    >
                        Salvar Configuração
                    </button>
                </div>
            </div>

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
