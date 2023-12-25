define skip-instr
python
import re
resp = gdb.execute('x/2i $pc', to_string=True)
resp = re.findall(r'(0x[0-9A-Fa-f]+)(?=(?: <[^>]+>)?:)', resp)
gdb.execute(f'set $pc={resp[1]}')
end
end

define break-next-instr
python
import re
resp = gdb.execute('x/2i $pc', to_string=True)
resp = re.findall(r'(0x[0-9A-Fa-f]+)(?=(?: <[^>]+>)?:)', resp)
gdb.execute(f'b *{resp[1]}', to_string=True)
end
end

alias ski = skip-instr
alias bni = break-next-instr
