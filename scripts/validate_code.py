import scripts.commit_validation.validate_type_id as type_id
import scripts.commit_validation.validate_headers as headers

print("Running Ferrum3D source validation...")

print("===> Validate type UUID in RTTI declarations")
type_id.process_files()
print()

print("===> Validate include guards in header files")
headers.process_files()
print()
