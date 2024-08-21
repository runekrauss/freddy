from pprint import pformat

class EdgePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return str(self.val.__dir__())

def freddy_pretty_printer_function(val):
    if "edge" in str(val.type) or "node" in str(val.type):
        print(val.type)
    if str(val.type) == 'std::__1::shared_ptr<freddy::detail::edge<bool, bool> >': return EdgePrinter(val)


gdb.pretty_printers.append(freddy_pretty_printer_function)