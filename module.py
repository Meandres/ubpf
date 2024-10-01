from osv.modules.filemap import FileMap

usr_files = FileMap()
usr_files.add('${OSV_BASE}/modules/ubpf/build/lib/').to('/usr')
