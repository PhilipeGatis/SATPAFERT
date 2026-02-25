import type { AQStatus } from '../App';

function Badge({ label, on, texts }: { label: string; on?: boolean; texts: [string, string] }) {
    if (on === undefined) return null;
    return (
        <div className="flex items-center justify-between border-b border-border/50 py-3 last:border-0">
            <span className="text-sm font-medium tracking-wide text-muted">{label}</span>
            <span className={`rounded-full px-3 py-1 text-xs font-bold tracking-wider ${on ? 'bg-accent/20 text-accent' : 'bg-white/5 text-muted'}`}>
                {on ? texts[0] : texts[1]}
            </span>
        </div>
    );
}

export default function HomeTab({ status }: { status: AQStatus | null }) {
    const shortDays = ['D', 'S', 'T', 'Q', 'Q', 'S', 'S'];

    const renderFertTable = () => {
        if (!status?.stocks) return <div className="text-xs text-muted">Aguardando dados...</div>;

        // Find stocks that have at least one dose > 0, excluding Prime (index 4)
        const activeStocks = status.stocks
            .map((s, idx) => ({ ...s, originalIndex: idx }))
            .filter(s => s.originalIndex !== 4 && s.doses?.some(d => Number(d) > 0));

        if (activeStocks.length === 0) {
            return (
                <div className="text-xs font-semibold text-muted italic mt-1">Nenhum agendamento ativo na semana.</div>
            );
        }

        return (
            <div className="overflow-x-auto mt-2 rounded bg-black/20 p-2">
                <table className="w-full text-left border-collapse">
                    <thead>
                        <tr className="border-b border-white/10 text-[10px] text-muted uppercase tracking-wider">
                            <th className="py-2 pr-2 font-medium">Canal</th>
                            <th className="py-2 px-1 font-medium text-center">Hora</th>
                            {shortDays.map((d, i) => (
                                <th key={i} className="py-2 px-1 font-medium text-center">{d}</th>
                            ))}
                        </tr>
                    </thead>
                    <tbody className="text-xs">
                        {activeStocks.map((s, i) => (
                            <tr key={i} className="border-b border-white/5 last:border-0">
                                <td className="py-3 pr-2 font-medium text-text whitespace-nowrap">
                                    {s.name || `CH ${s.originalIndex + 1}`}
                                </td>
                                <td className="py-3 px-1 font-mono text-muted text-center tracking-tighter">
                                    {String(s.sH || 0).padStart(2, '0')}:{String(s.sM || 0).padStart(2, '0')}
                                </td>
                                {[0, 1, 2, 3, 4, 5, 6].map(di => {
                                    const vol = s.doses?.[di] || 0;
                                    return (
                                        <td key={di} className="py-3 px-1 text-center">
                                            {Number(vol) > 0 ? (
                                                <span className="font-bold text-accent2">{Number(vol)}</span>
                                            ) : (
                                                <span className="text-muted/30">-</span>
                                            )}
                                        </td>
                                    );
                                })}
                            </tr>
                        ))}
                    </tbody>
                </table>
            </div>
        );
    };

    return (
        <div className="flex flex-col gap-4">
            {/* Sensors Card */}
            <div className="rounded-2xl bg-card p-5 shadow-md">
                <h2 className="mb-4 text-base font-medium tracking-wide text-text/90 uppercase">Sensores e Segurança</h2>

                <div className="mb-5">
                    <div className="mb-2 flex justify-between text-sm font-medium">
                        <span className="text-muted tracking-wide">Nível de Água</span>
                        <span className={`font-mono text-lg ${status?.waterLevel && status.waterLevel < 5 ? 'text-danger' : status?.waterLevel && status.waterLevel < 10 ? 'text-warn' : 'text-accent2'}`}>
                            {status?.waterLevel?.toFixed(1) || '--'} cm
                        </span>
                    </div>
                    <div className="h-2 w-full overflow-hidden rounded-full bg-white/5">
                        <div
                            className={`h-full transition-all duration-500 ease-out ${status?.waterLevel && status.waterLevel < 5 ? 'bg-danger' : status?.waterLevel && status.waterLevel < 10 ? 'bg-warn' : 'bg-accent2'}`}
                            style={{ width: `${Math.min(100, Math.max(0, 100 - (status?.waterLevel || 0) * 5))}%` }}
                        />
                    </div>
                </div>

                <div className="flex flex-col">
                    <Badge label="Nível Ótico" on={status?.optical} texts={['CHEIO', 'NORMAL']} />
                    <Badge label="Bóia de Reposição" on={status?.float} texts={['VAZIO', 'NO NÍVEL']} />
                    <Badge label="Bomba Canister" on={status?.canister} texts={['LIGADA', 'DESL.']} />
                </div>
            </div>

            {/* Routine State Card */}
            <div className="rounded-2xl bg-card p-5 shadow-md">
                <h2 className="mb-4 text-base font-medium tracking-wide text-text/90 uppercase">Estado do Sistema</h2>
                <div className="flex flex-col">
                    <Badge label="Estado TPA" on={status?.tpaState !== 'IDLE'} texts={[status?.tpaState || '', 'IDLE']} />
                    <Badge label="Modo Manutenção" on={status?.maintenance} texts={['ATIVO', 'INATIVO']} />
                </div>
            </div>

            {/* FERTILIZERS WEEKLY VIEW */}
            <div className="rounded-2xl bg-card p-5 shadow-md">
                <div className="mb-4 flex items-center justify-between">
                    <h2 className="text-base font-medium tracking-wide text-text/90 uppercase">Tabela de Fertilizantes</h2>
                    <span className="text-[10px] font-bold text-accent2 uppercase tracking-wider bg-accent2/10 px-2 py-1 rounded-full border border-accent2/20">Em mL</span>
                </div>
                {renderFertTable()}
            </div>
        </div>
    );
}
