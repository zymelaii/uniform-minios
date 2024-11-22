define skip-instr
python
import re
resp = gdb.execute('x/2i $pc', to_string=True)
resp = re.findall(r'(0x[0-9A-Fa-f]+)(?=(?: <[^>]+>)?:)', resp)
gdb.execute(f'set $pc={resp[1]}')
end
tui disable
tui enable
end

define break-next-instr
python
import re
resp = gdb.execute('x/2i $pc', to_string=True)
resp = re.findall(r'(0x[0-9A-Fa-f]+)(?=(?: <[^>]+>)?:)', resp)
gdb.execute(f'break *{resp[1]}', to_string=True)
end
end

define nexti-anyhow
python
import re
instrs = gdb.execute('x/2i $pc', to_string=True).strip().split('\n')
pat = r'(0x[0-9A-Fa-f]+):(?:[^\w]+)(\w+)'
jmp_opcodes = [
    'jmp',
    'ret',
    'ja', 'jb', 'jc', 'je', 'jg', 'jl', 'jo', 'jp',
    'js', 'jz',
    'jbe', 'jae', 'jge', 'jle', 'jna', 'jnb', 'jnc', 'jne',
    'jng', 'jnl', 'jno', 'jnp', 'jns', 'jnz', 'jpe', 'jpo',
    'jnle', 'jnbe', 'jnge', 'jcxz', 'jnae',
    'jecxz',
]
if (opcode := re.search(pat, instrs[0]).group(2)) in jmp_opcodes:
    print(f'warn: next instr is unreachble in one step, execute stepi instead')
    gdb.execute('stepi', to_string=True)
else:
    addr = re.search(pat, instrs[1]).group(1)
    gdb.execute(f'tbreak *{addr}')
    gdb.execute('continue')
    gdb.execute('tui refresh')
end
end

alias ski = skip-instr
alias bni = break-next-instr
alias nix = nexti-anyhow
