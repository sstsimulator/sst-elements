import sst
try:
    import sst.memHierarchy  # noqa: F401
except ModuleNotFoundError:
    pass
sst.Component("ecc_model_test", "carcosa.EccModelTest")
