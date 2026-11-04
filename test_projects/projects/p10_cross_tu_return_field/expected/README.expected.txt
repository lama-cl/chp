Expected C3a behavior:
- Imported return origin should pass through a simple field store and field read.
- This exercises existing fieldOriginState/fieldValueFlows with imported call
  return origins.
- If this fails, field-store integration needs a later targeted patch.
