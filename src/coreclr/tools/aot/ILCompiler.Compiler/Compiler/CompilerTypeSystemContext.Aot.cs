// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.Runtime.CompilerServices;

using Internal.TypeSystem;
using Internal.IL;

using Interlocked = System.Threading.Interlocked;

namespace ILCompiler
{
    public partial class CompilerTypeSystemContext
    {
        // Chosen rather arbitrarily. For the app that I was looking at, cutoff point of 7 compiled
        // more than 10 minutes on a release build of the compiler, and I lost patience.
        // Cutoff point of 5 produced an 1.7 GB object file.
        // Cutoff point of 4 produced an 830 MB object file.
        // Cutoff point of 3 produced an 470 MB object file.
        // We want this to be high enough so that it doesn't cut off too early. But also not too
        // high because things that are recursive often end up expanding laterally as well
        // through various other generic code the deep code calls into.
        public const int DefaultGenericCycleDepthCutoff = 4;

        public const int DefaultGenericCycleBreadthCutoff = 10;

        public SharedGenericsConfiguration GenericsConfig
        {
            get;
        }

        private readonly MetadataFieldLayoutAlgorithm _metadataFieldLayoutAlgorithm = new CompilerMetadataFieldLayoutAlgorithm();
        private readonly RuntimeDeterminedFieldLayoutAlgorithm _runtimeDeterminedFieldLayoutAlgorithm = new RuntimeDeterminedFieldLayoutAlgorithm();
        private readonly VectorOfTFieldLayoutAlgorithm _vectorOfTFieldLayoutAlgorithm;
        private readonly VectorFieldLayoutAlgorithm _vectorFieldLayoutAlgorithm;
        private readonly Int128FieldLayoutAlgorithm _int128FieldLayoutAlgorithm;
        private readonly TypeWithRepeatedFieldsFieldLayoutAlgorithm _typeWithRepeatedFieldsFieldLayoutAlgorithm;

        private TypeDesc[] _arrayOfTInterfaces;
        private TypeDesc[] _arrayEnumeratorOfTInterfaces;
        private ArrayOfTRuntimeInterfacesAlgorithm _arrayOfTRuntimeInterfacesAlgorithm;
        private MetadataType _arrayOfTType;
        private MetadataType _arrayOfTEnumeratorType;
        private MetadataType _attributeType;

        public CompilerTypeSystemContext(TargetDetails details, SharedGenericsMode genericsMode, DelegateFeature delegateFeatures,
            int genericCycleDepthCutoff = DefaultGenericCycleDepthCutoff,
            int genericCycleBreadthCutoff = DefaultGenericCycleBreadthCutoff)
            : base(details)
        {
            _genericsMode = genericsMode;

            _vectorOfTFieldLayoutAlgorithm = new VectorOfTFieldLayoutAlgorithm(_metadataFieldLayoutAlgorithm);
            _vectorFieldLayoutAlgorithm = new VectorFieldLayoutAlgorithm(_metadataFieldLayoutAlgorithm);
            _int128FieldLayoutAlgorithm = new Int128FieldLayoutAlgorithm(_metadataFieldLayoutAlgorithm);
            _typeWithRepeatedFieldsFieldLayoutAlgorithm = new TypeWithRepeatedFieldsFieldLayoutAlgorithm(_metadataFieldLayoutAlgorithm);

            _delegateInfoHashtable = new DelegateInfoHashtable(delegateFeatures);

            _genericCycleDetector = new LazyGenericsSupport.GenericCycleDetector(genericCycleDepthCutoff, genericCycleBreadthCutoff);

            GenericsConfig = new SharedGenericsConfiguration();
        }

        public MetadataType ArrayOfTEnumeratorType
        {
            get
            {
                // This type is optional, but it's fine for this cache to be ineffective if that happens.
                // Those scenarios are rare and typically deal with small compilations.
                return _arrayOfTEnumeratorType ??= SystemModule.GetType("System", "SZGenericArrayEnumerator`1", throwIfNotFound: false);
            }
        }

        public bool IsArrayVariantCastable(TypeDesc type)
        {
            // Arrays and array enumerators have weird casting rules due to array covariance
            // (string[] castable to object[], or int[] castable to uint[], or int[] castable
            // to IList<SomeIntEnum>).

            if (type.IsSzArray
                || type.GetTypeDefinition() == ArrayOfTEnumeratorType
                || IsGenericArrayInterfaceType(type)
                || IsGenericArrayEnumeratorInterfaceType(type))
            {
                TypeDesc elementType = type.IsSzArray ? ((ArrayType)type).ElementType : type.Instantiation[0];
                if (CastingHelper.IsArrayElementTypeCastableBySize(elementType) ||
                    (elementType.IsDefType && !elementType.IsValueType))
                {
                    return true;
                }
            }

            return false;
        }

        protected override RuntimeInterfacesAlgorithm GetRuntimeInterfacesAlgorithmForNonPointerArrayType(ArrayType type)
        {
            _arrayOfTRuntimeInterfacesAlgorithm ??= new ArrayOfTRuntimeInterfacesAlgorithm(SystemModule.GetKnownType("System", "Array`1"));
            return _arrayOfTRuntimeInterfacesAlgorithm;
        }

        public override FieldLayoutAlgorithm GetLayoutAlgorithmForType(DefType type)
        {
            if (type.IsRuntimeDeterminedType)
                return _runtimeDeterminedFieldLayoutAlgorithm;
            else if (VectorOfTFieldLayoutAlgorithm.IsVectorOfTType(type))
                return _vectorOfTFieldLayoutAlgorithm;
            else if (VectorFieldLayoutAlgorithm.IsVectorType(type))
                return _vectorFieldLayoutAlgorithm;
            else if (Int128FieldLayoutAlgorithm.IsIntegerType(type))
                return _int128FieldLayoutAlgorithm;
            else if (type is TypeWithRepeatedFields)
                return _typeWithRepeatedFieldsFieldLayoutAlgorithm;
            else
                return _metadataFieldLayoutAlgorithm;
        }

        protected override bool ComputeHasGCStaticBase(FieldDesc field)
        {
            Debug.Assert(field.IsStatic);

            if (field.IsThreadStatic)
                return true;

            TypeDesc fieldType = field.FieldType;
            if (fieldType.IsValueType)
                return ((DefType)fieldType).ContainsGCPointers;
            else
                return fieldType.IsGCPointer;
        }

        /// <summary>
        /// Returns true if <paramref name="type"/> is a generic interface type implemented by arrays.
        /// </summary>
        public bool IsGenericArrayInterfaceType(TypeDesc type)
        {
            // Hardcode the fact that all generic interfaces on array types have arity 1
            if (!type.IsInterface || type.Instantiation.Length != 1)
                return false;

            if (_arrayOfTInterfaces == null)
            {
                DefType[] implementedInterfaces = SystemModule.GetKnownType("System", "Array`1").ExplicitlyImplementedInterfaces;
                TypeDesc[] interfaceDefinitions = new TypeDesc[implementedInterfaces.Length];
                for (int i = 0; i < interfaceDefinitions.Length; i++)
                    interfaceDefinitions[i] = implementedInterfaces[i].GetTypeDefinition();
                Interlocked.CompareExchange(ref _arrayOfTInterfaces, interfaceDefinitions, null);
            }

            TypeDesc interfaceDefinition = type.GetTypeDefinition();
            foreach (var arrayInterfaceDefinition in _arrayOfTInterfaces)
            {
                if (interfaceDefinition == arrayInterfaceDefinition)
                    return true;
            }

            return false;
        }

        public bool IsGenericArrayEnumeratorInterfaceType(TypeDesc type)
        {
            // Hardcode the fact that all generic interfaces on array enumerator have arity 1
            if (!type.IsInterface || type.Instantiation.Length != 1)
                return false;

            if (_arrayEnumeratorOfTInterfaces == null)
            {
                DefType[] implementedInterfaces = ArrayOfTEnumeratorType?.ExplicitlyImplementedInterfaces;

                ArrayBuilder<TypeDesc> definitions = default;
                if (implementedInterfaces != null)
                {
                    foreach (DefType interfaceType in implementedInterfaces)
                    {
                        if (interfaceType.HasInstantiation)
                            definitions.Add(interfaceType.GetTypeDefinition());
                    }
                }

                Interlocked.CompareExchange(ref _arrayEnumeratorOfTInterfaces, definitions.ToArray(), null);
            }

            TypeDesc interfaceDefinition = type.GetTypeDefinition();
            return Array.IndexOf(_arrayEnumeratorOfTInterfaces, interfaceDefinition) >= 0;
        }

        protected override IEnumerable<MethodDesc> GetAllMethods(TypeDesc type)
        {
            return GetAllMethods(type, virtualOnly: false);
        }

        protected override IEnumerable<MethodDesc> GetAllVirtualMethods(TypeDesc type)
        {
            return GetAllMethods(type, virtualOnly: true);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private IEnumerable<MethodDesc> GetAllMethods(TypeDesc type, bool virtualOnly)
        {
            MetadataType attributeType = _attributeType ??= SystemModule.GetType("System", "Attribute");

            if (type.IsDelegate)
            {
                return GetAllMethodsForDelegate(type, virtualOnly);
            }
            else if (type.IsEnum)
            {
                return GetAllMethodsForEnum(type, virtualOnly);
            }
            else if (type.IsValueType)
            {
                return GetAllMethodsForValueType(type, virtualOnly);
            }
            else if (type.CanCastTo(attributeType))
            {
                return GetAllMethodsForAttribute(type, virtualOnly);
            }

            return virtualOnly ? type.GetVirtualMethods() : type.GetMethods();
        }

        protected virtual IEnumerable<MethodDesc> GetAllMethodsForDelegate(TypeDesc type, bool virtualOnly)
        {
            // Inject the synthetic methods that support the implementation of the delegate.
            InstantiatedType instantiatedType = type as InstantiatedType;
            if (instantiatedType != null)
            {
                DelegateInfo info = GetDelegateInfo(type.GetTypeDefinition());
                foreach (MethodDesc syntheticMethod in info.Methods)
                {
                    if (!virtualOnly || syntheticMethod.IsVirtual)
                        yield return GetMethodForInstantiatedType(syntheticMethod, instantiatedType);
                }
            }
            else
            {
                DelegateInfo info = GetDelegateInfo(type);
                foreach (MethodDesc syntheticMethod in info.Methods)
                {
                    if (!virtualOnly || syntheticMethod.IsVirtual)
                        yield return syntheticMethod;
                }
            }

            // Append all the methods defined in metadata
            IEnumerable<MethodDesc> metadataMethods = virtualOnly ? type.GetVirtualMethods() : type.GetMethods();
            foreach (var m in metadataMethods)
                yield return m;
        }

        internal DefType GetClosestDefType(TypeDesc type)
        {
            if (type.IsArray)
            {
                if (!type.IsArrayTypeWithoutGenericInterfaces())
                {
                    MetadataType arrayShadowType = _arrayOfTType ??= SystemModule.GetType("System", "Array`1");
                    return arrayShadowType.MakeInstantiatedType(((ArrayType)type).ElementType);
                }

                return GetWellKnownType(WellKnownType.Array);
            }

            Debug.Assert(type is DefType);
            return (DefType)type;
        }

        private readonly LazyGenericsSupport.GenericCycleDetector _genericCycleDetector;

        public void DetectGenericCycles(TypeSystemEntity owner, TypeSystemEntity referent)
        {
            _genericCycleDetector.DetectCycle(owner, referent);
        }

        public void LogWarnings(Logger logger)
        {
            _genericCycleDetector.LogWarnings(logger);
        }
    }

    public class SharedGenericsConfiguration
    {
        // To avoid infinite generic recursion issues during debug type record generation, attempt to
        // use canonical form for types with high generic complexity.
        public long MaxGenericDepthOfDebugRecord { get; }

        public SharedGenericsConfiguration()
        {
            MaxGenericDepthOfDebugRecord = 15;
        }
    }
}
